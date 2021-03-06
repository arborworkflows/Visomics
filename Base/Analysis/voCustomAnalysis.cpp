/*=========================================================================

  Program: Visomics

  Copyright (c) Kitware, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Qt includes
#include <QDebug>

// QtPropertyBrowser includes
#include <QtVariantProperty>
#include <QtVariantPropertyManager>

// Visomics includes
#include "voCustomAnalysis.h"
#include "voCustomAnalysisInformation.h"
#include "voDataObject.h"
#include "voInputFileDataObject.h"
#include "voTableDataObject.h"
#include "voUtils.h"
#include "vtkExtendedTable.h"

// VTK includes
#include <vtkArrayData.h>
#include <vtkCompositeDataIterator.h>
#include <vtkDoubleArray.h>
#include <vtkGraph.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTableToGraph.h>
#include <vtkTree.h>

// --------------------------------------------------------------------------
// voCustomAnalysisPrivate methods

// --------------------------------------------------------------------------
class voCustomAnalysisPrivate
{
public:
  voCustomAnalysisInformation *Information;
};

// --------------------------------------------------------------------------
// voCustomAnalysis methods

// --------------------------------------------------------------------------
voCustomAnalysis::voCustomAnalysis(QObject* newParent):
    Superclass(), d_ptr(new voCustomAnalysisPrivate)
{
  if (newParent)
    {
    this->setParent(newParent);
    }
  Q_D(voCustomAnalysis);
  d->Information = NULL;
}

// --------------------------------------------------------------------------
voCustomAnalysis::~voCustomAnalysis()
{
}

// --------------------------------------------------------------------------
void voCustomAnalysis::loadInformation(voCustomAnalysisInformation *info)
{
  Q_D(voCustomAnalysis);
  d->Information = info;
}

// --------------------------------------------------------------------------
void voCustomAnalysis::setOutputInformation()
{
  Q_D(voCustomAnalysis);
  if (d->Information == NULL)
    {
    return;
    }
  foreach(voCustomAnalysisData *output, d->Information->outputs())
    {
    QString name = output->name();
    QString type = output->type();

    QString viewType = "";
    QString viewPrettyName = "";
    QString rawViewType = "";
    QString rawViewPrettyName = "";
    QList< QPair <QString, QString> > views = output->views();
    if (views.size() > 0)
      {
      rawViewPrettyName = views.at(0).first;
      rawViewType = views.at(0).second;
      }
    if (views.size() > 1)
      {
      viewPrettyName = views.at(1).first;
      viewType = views.at(1).second;
      }

    if (type == "Table")
      {
      this->addOutputType(name, "vtkTable",
                          viewType, viewPrettyName,
                          rawViewType, rawViewPrettyName);
      }
    else if (type == "Tree")
      {
      this->addOutputType(name, "vtkTree",
                          viewType, viewPrettyName,
                          rawViewType, rawViewPrettyName);
      }
    else
      {
      qCritical() << "Unsupported output type in voCustomAnalysis:" << type;
      }

    if (views.size() > 2)
      {
      for (int i = 2; i < views.size(); ++i)
        {
        viewPrettyName = views.at(i).first;
        viewType = views.at(i).second;
        this->addOutputView(name, viewType, viewPrettyName);
        }
      }

    }
}

// --------------------------------------------------------------------------
void voCustomAnalysis::setParameterInformation()
{
  Q_D(voCustomAnalysis);
  if (d->Information == NULL)
    {
    return;
    }

  // first handle the case where this custom analysis has no parameters.
  if (d->Information->parameters().size() == 0)
    {
    this->parameterDescriptions = "";
    return;
    }

  QList<QtProperty*> custom_parameters;
  this->parameterDescriptions = "<dl>";
  foreach(voCustomAnalysisParameter *parameter, d->Information->parameters())
    {
    QString type = parameter->type();
    QString name = parameter->name();
    QString parameterTitle = "";
    QStringList enum_options;
    int tableIndex = -1;

    QVariant minValue(VTK_INT_MIN);
    QVariant maxValue(VTK_INT_MAX);
    QVariant defaultValue(0);

    // read fields for this parameter
    foreach(voCustomAnalysisParameterField *field, parameter->fields())
      {
      if (field->name() == "title")
        {
        parameterTitle = field->value();
        }
      else if (field->name() == "description")
        {
        this->parameterDescriptions.append(
          QString("<dt><b>%1:</b></dt>").arg(parameterTitle));
        this->parameterDescriptions.append(
          QString("<dd>%1</dd>").arg(field->value()));
        }
      else if (field->name() == "min")
        {
        minValue = QVariant(field->value().toDouble());
        }
      else if (field->name() == "max")
        {
        maxValue = QVariant(field->value().toDouble());
        }
      else if (field->name() == "default")
        {
        if (type == "String" || type == "Range")
          {
          defaultValue = QVariant(field->value());
          }
        else
          {
          defaultValue = QVariant(field->value().toDouble());
          }
        }
      else if (field->name() == "option")
        {
        enum_options << field->value();
        }
      else if (field->name() == "table")
        {
        int index = 0;
        // find the index of the table named by this parameter
        foreach(voCustomAnalysisData *input, d->Information->inputs())
          {
          if (input->name() == field->value())
            {
            break;
            }
          ++index;
          }
        tableIndex = index;
        }
      }

    // set up this parameter, based on what type it is
    if (type == "Integer")
      {
      custom_parameters << this->addIntegerParameter(
        parameter->name(), parameterTitle,
        minValue.toInt(), maxValue.toInt(), defaultValue.toInt());
      }
    else if (type == "Double")
      {
      custom_parameters << this->addIntegerParameter(
        parameter->name(), parameterTitle,
        minValue.toDouble(), maxValue.toDouble(), defaultValue.toDouble());
      }
    else if (type == "String" || type == "Range")
      {
      custom_parameters << this->addStringParameter(
        parameter->name(), parameterTitle, defaultValue.toString());
      }
    else if (type == "Enum")
      {
      custom_parameters << this->addEnumParameter(
        parameter->name(), parameterTitle, enum_options);
      }
    else if (type == "Column")
      {
      this->addColumnParameter(parameter->name(), parameterTitle, tableIndex);
      }
    else
      {
      qCritical() << "Unsupported parameter type in voCustomAnalysis:" << type;
      }
    }
  this->parameterDescriptions.append("</dl>");

  this->addParameterGroup(QString("%1 parameters").arg(this->objectName()),
                          custom_parameters);
}

// --------------------------------------------------------------------------
QString voCustomAnalysis::parameterDescription()const
{
  return this->parameterDescriptions;
}

// --------------------------------------------------------------------------
int voCustomAnalysis::execute()
{
  return voAnalysis::SUCCESS;
}

voCustomAnalysisInformation *voCustomAnalysis::information() const
{
  return d_ptr->Information;
}
