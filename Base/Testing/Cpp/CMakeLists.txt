
SET(KIT ${PROJECT_NAME})

CREATE_TEST_SOURCELIST(Tests ${KIT}CppTests.cpp
  voAnalysisTest.cpp
  voApplicationTest.cpp
  voDataObjectTest.cpp
  voUtilsTest.cpp
  vtkExtendedTableTest.cpp
  )

SET(TestsToRun ${Tests})
REMOVE(TestsToRun ${KIT}CppTests.cpp)

ADD_EXECUTABLE(${KIT}CppTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CppTests ${PROJECT_NAME}Lib)

MACRO(SIMPLE_TEST TESTNAME)
  ADD_TEST(NAME ${TESTNAME}
    COMMAND ${Visomics_LAUNCH_COMMAND} $<TARGET_FILE:${KIT}CppTests> ${TESTNAME} ${ARGN})
  #SET_PROPERTY(TEST ${TESTNAME} PROPERTY LABELS ${PROJECT_NAME})
ENDMACRO()

SIMPLE_TEST(voAnalysisTest)
SIMPLE_TEST(voApplicationTest ${Visomics_BINARY_DIR})
SIMPLE_TEST(voDataObjectTest)
SIMPLE_TEST(voUtilsTest)
SIMPLE_TEST(vtkExtendedTableTest)
