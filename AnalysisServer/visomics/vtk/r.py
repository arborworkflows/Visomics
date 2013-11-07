# import vtk wrapped version that will raise exceptions for error events
import vtkwithexceptions as vtk
import json
import os

from celery import Celery
from celery import task, current_task
from celery.result import AsyncResult

celery = Celery()
celery.config_from_object('celeryconfig')

@celery.task
def run(input):
    task_description =  parse_json(input);

    return execute(task_description['inputs'], task_description['outputs'],
                   task_description['script'])

def execute(inputs, outputs, script):

    # Setup the pipeline
    rcalc = vtk.vtkRCalculatorFilter()
    rcalc.SetRscript(script)

    readers = []

    for name, type, value in inputs:

        if type == 'Table':
            rcalc.PutTable(name)
            input = vtk.vtkTableReader()
        elif type == 'Tree':
            rcalc.PutTree(name)
            input = vtk.vtkTreeReader()

        input.SetInputString(value)
        input.SetReadFromInputString(1)

        readers.append(input)

    # Multi inputs
    if len(inputs) > 1:
        input = vtk.vtkMultiBlockDataGroupFilter()

        for reader in readers:
            input.AddInputConnection(reader.GetOutputPort())

    rcalc.AddInputConnection(input.GetOutputPort())

    for o in outputs:
        if o['type'] == 'Table':
            rcalc.GetTable(o['name'])
        elif o['type'] == 'Tree':
            rcalc.GetTree(o['name'])

    rcalc.Update()

    output = rcalc.GetOutput()

    output_dataobjects = []

    if len(outputs) > 1:
        iter = output.NewIterator();
        iter.InitTraversal()
        while not iter.IsDoneWithTraversal():
            dataobject = iter.GetCurrentDataObject()
            output_dataobjects.append(dataobject)
            iter.GoToNextItem()
    else:
        output_dataobjects.append(output)

    output_json  =  {'output': []}

    index = 0

    for dataobject in output_dataobjects:

        output = outputs[index]
        index += 1

        if output['type'] == 'Table':
            writer = vtk.vtkTableWriter()
        elif output['type'] == 'Tree':
            writer = vtk.vtkTreeWriter()

        writer.SetWriteToOutputString(1)
        writer.SetInputData(dataobject)
        writer.Update()

        data = writer.GetOutputString()

        output_json['output'].append({'name': output['name'], 'type': output['type'], 'data': data})

    return output_json

def parse_json(json_string):
    job_descriptor = json.loads(json_string)

    inputs = job_descriptor['inputs']

    job = {'name': job_descriptor['name'],
           'script': job_descriptor['script'],
           'outputs': job_descriptor['outputs']}

    parsed_inputs = []

    for input in inputs:
        type = input['type']
        if not type:
            raise Exception("Input is missing type property")

        data = input['data']

        name = input['name']
        if not name:
            raise Exception("Input is missing name property")

        parsed_inputs.append((name, type, data))

    job['inputs'] = parsed_inputs

    return job