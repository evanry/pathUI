PROJECT(BASICFILTERS)

include(GenerateExportHeader)

INCLUDE_DIRECTORIES(
  ${MULTIRESOLUTIONIMAGEINTERFACE_SOURCE_DIR}
  ../../
  ${Boost_INCLUDE_DIRS}
  ${SUPERPIXELCLASSSIFICATION_SOURCE_DIR}
  ${DIAGPathology_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CORE_BINARY_DIR}
  ${MULTIRESOLUTIONIMAGEINTERFACE_BINARY_DIR}
)
ADD_DEFINITIONS(${Boost_DEFINITIONS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

SET(BASICFILTERS_SRCS 
  FilterBase.cpp
  ImageFilter.cpp
  ColorDeconvolutionFilter.cpp
)
SET(BASICFILTERS_HS
  FilterBase.h
  ImageFilter.h
  ColorDeconvolutionFilter.h
)

ADD_LIBRARY(basicfilters SHARED ${BASICFILTERS_SRCS} ${BASICFILTERS_HS})
GENERATE_EXPORT_HEADER(basicfilters)
TARGET_LINK_LIBRARIES(basicfilters multiresolutionimageinterface core)


SET_TARGET_PROPERTIES(basicfilters PROPERTIES DEBUG_POSTFIX _d)


INSTALL(FILES FilterBase.h ColorDeconvolutionFilter.h ImageFilter.h DESTINATION include/imgproc/basicfilters)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/basicfilters_export.h DESTINATION include/imgproc/basicfilters)

INSTALL(TARGETS basicfilters
  EXPORT asap-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

IF(WIN32)  
  SET_TARGET_PROPERTIES(basicfilters PROPERTIES FOLDER pathology/diag/imgproc)    
ENDIF(WIN32)