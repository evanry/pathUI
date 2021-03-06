PROJECT(ANNOTATION)

include(GenerateExportHeader)

if(POLICY CMP0053)
      cmake_policy(SET CMP0053 NEW) # CMake 3.1
    endif()

INCLUDE_DIRECTORIES(
  .
  ../
  ../io/multiresolutionimageinterface
  ${Boost_INCLUDE_DIRS}
  ${PugiXML_INCLUDE_DIR}
  ${DIAGPathology_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CORE_BINARY_DIR}
  ${MULTIRESOLUTIONIMAGEINTERFACE_BINARY_DIR}
)
LINK_DIRECTORIES(${Boost_LIBRARY_DIR})
ADD_DEFINITIONS(${Boost_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

SET(ANNOTATION_HEADERS
  Annotation.h
  AnnotationBase.h
  AnnotationToMask.h
  AnnotationGroup.h
  AnnotationList.h
  AnnotationService.h
  XmlRepository.h
  NDPARepository.h
  ImageScopeRepository.h
  Repository.h
)

SET(ANNOTATION_SOURCE
  Annotation.cpp
  AnnotationBase.cpp
  AnnotationGroup.cpp
  AnnotationToMask.cpp
  AnnotationList.cpp
  AnnotationService.cpp
  XmlRepository.cpp
  NDPARepository.cpp
  ImageScopeRepository.cpp
  Repository.cpp
)

add_library(annotation SHARED ${ANNOTATION_SOURCE} ${ANNOTATION_HEADERS})
generate_export_header(annotation)
TARGET_LINK_LIBRARIES(annotation pugixml core multiresolutionimageinterface)
SET_TARGET_PROPERTIES(annotation PROPERTIES DEBUG_POSTFIX _d)
#SET_TARGET_PROPERTIES(annotation PROPERTIES RELEASE_POSTFIX "")

INSTALL(TARGETS annotation
  EXPORT asap-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

INSTALL(FILES ${ANNOTATION_HEADERS} DESTINATION include/annotation)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/annotation_export.h DESTINATION include/annotation)

IF(WIN32)
  SET_TARGET_PROPERTIES(annotation PROPERTIES FOLDER pathology/diag)   
ENDIF(WIN32)
