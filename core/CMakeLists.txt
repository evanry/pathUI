PROJECT(CORE)

include(GenerateExportHeader)

ADD_DEFINITIONS(${Boost_DEFINITIONS})

IF (WIN32)
  ADD_DEFINITIONS(-DBUILD_CORE)
ENDIF (WIN32)

INCLUDE_DIRECTORIES(
.
../
${Boost_INCLUDE_DIRS}
${DIAGPathology_BINARY_DIR}
${CMAKE_CURRENT_BINARY_DIR}
)

LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

SET(core_src filetools.cpp stringconversion.cpp PathologyEnums.cpp ImageSource.cpp Patch.hpp Box.cpp Point.cpp ProgressMonitor.cpp CmdLineProgressMonitor.cpp)
SET(core_headers filetools.h stringconversion.h PathologyEnums.h ImageSource.h Patch.h Patch.hpp Box.h Point.h ProgressMonitor.h CmdLineProgressMonitor.h)

ADD_LIBRARY(core SHARED ${core_src} ${core_headers})
generate_export_header(core)
TARGET_Link_libraries(core ${Boost_LIBRARIES})

SET_TARGET_PROPERTIES(core PROPERTIES DEBUG_POSTFIX _d)
#SET_TARGET_PROPERTIES(core PROPERTIES RELEASE_POSTFIX "")
IF(WIN32)  
  SET_TARGET_PROPERTIES(core PROPERTIES FOLDER diag)    
ENDIF(WIN32)

INSTALL(TARGETS core
  EXPORT asap-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

INSTALL(FILES ${core_headers} DESTINATION include/core)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/core_export.h DESTINATION include/core)

IF(WIN32)
  SET_TARGET_PROPERTIES(core PROPERTIES FOLDER pathology/diag)   
ENDIF(WIN32)
