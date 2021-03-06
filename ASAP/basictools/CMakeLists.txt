PROJECT(BasicTools)

include(GenerateExportHeader)
if(POLICY CMP0053)
      cmake_policy(SET CMP0053 NEW) # CMake 3.1
    endif()

INCLUDE_DIRECTORIES(
../
../../
${DIAGPathology_BINARY_DIR}
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(ZoomTool_SOURCE 
  ZoomTool.h
  ZoomTool.cpp
)

set(RulerTool_SOURCE
  RulerTool.h
  RulerTool.cpp
)

set(PanTool_SOURCE 
  PanTool.h
  PanTool.cpp
)


IF(BUILD_USING_QT4)
  QT4_WRAP_CPP(PanTool_MOC PanTool.h)
  QT4_WRAP_CPP(ZoomTool_MOC ZoomTool.h)
  qt4_add_resources(TOOL_RESOURCES ${BasicTools_SOURCE_DIR}/resources/basictools_resources.qrc)
  ADD_LIBRARY(PanToolPlugin SHARED ${PanTool_SOURCE} ${PanTool_MOC} ${TOOL_RESOURCES})
  ADD_LIBRARY(ZoomToolPlugin SHARED ${ZoomTool_SOURCE} ${ZoomTool_MOC} ${TOOL_RESOURCES})
  TARGET_LINK_LIBRARIES(PanToolPlugin ${QT_LIBRARIES} )
  TARGET_LINK_LIBRARIES(ZoomToolPlugin ${QT_LIBRARIES} )
ELSE()
  find_package(Qt5 COMPONENTS Core Widgets Gui)  
  qt5_add_resources(TOOL_RESOURCES ${BasicTools_SOURCE_DIR}/resources/basictools_resources.qrc)
  ADD_LIBRARY(ZoomToolPlugin SHARED ${ZoomTool_SOURCE} ${TOOL_RESOURCES})
  QT5_USE_MODULES(ZoomToolPlugin Core Widgets)
  ADD_LIBRARY(RulerToolPlugin SHARED ${RulerTool_SOURCE} ${TOOL_RESOURCES})
  QT5_USE_MODULES(RulerToolPlugin Core Widgets)
  ADD_LIBRARY(PanToolPlugin SHARED ${PanTool_SOURCE} ${TOOL_RESOURCES})
  QT5_USE_MODULES(PanToolPlugin Core Widgets) 
ENDIF()

TARGET_LINK_LIBRARIES(PanToolPlugin ASAP)
TARGET_LINK_LIBRARIES(ZoomToolPlugin ASAP)
TARGET_LINK_LIBRARIES(RulerToolPlugin ASAP)
SET_TARGET_PROPERTIES(PanToolPlugin PROPERTIES DEBUG_POSTFIX _d)
SET_TARGET_PROPERTIES(ZoomToolPlugin PROPERTIES DEBUG_POSTFIX _d)
SET_TARGET_PROPERTIES(RulerToolPlugin PROPERTIES DEBUG_POSTFIX _d)
#SET_TARGET_PROPERTIES(PanToolPlugin PROPERTIES RELEASE_POSTFIX "")
#SET_TARGET_PROPERTIES(ZoomToolPlugin PROPERTIES RELEASE_POSTFIX "")
#SET_TARGET_PROPERTIES(RulerToolPlugin PROPERTIES RELEASE_POSTFIX "")

INSTALL(TARGETS PanToolPlugin ZoomToolPlugin RulerToolPlugin
  RUNTIME DESTINATION bin/plugins/tools
  LIBRARY DESTINATION bin/plugins/tools
  ARCHIVE DESTINATION lib/plugins/tools
)

INSTALL(FILES PanTool.h ZoomTool.h RulerTool.h DESTINATION include/ASAP/plugins/tools)

IF(WIN32)
  SET_TARGET_PROPERTIES(PanToolPlugin PROPERTIES FOLDER pathology/diag/ASAP/plugins/tools)   
  SET_TARGET_PROPERTIES(ZoomToolPlugin PROPERTIES FOLDER pathology/diag/ASAP/plugins/tools)   
  SET_TARGET_PROPERTIES(RulerToolPlugin PROPERTIES FOLDER pathology/diag/ASAP/plugins/tools)
ENDIF(WIN32)
