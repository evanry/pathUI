cmake_minimum_required(VERSION 2.8)
PROJECT(DIAGPathology)

SET(CURRENT_MAJOR_VERSION 1)
SET(CURRENT_MINOR_VERSION 6)
SET(CURRENT_PATCH_VERSION 0)
set(ASAP_VERSION ${CURRENT_MAJOR_VERSION}.${CURRENT_MINOR_VERSION}.${CURRENT_PATCH_VERSION})

include(GenerateExportHeader)

SET(OpenCV_DIR E:/download/opencv/build)
set(DCMTKJPEG_INCLUDE_DIR E:/download/ThirdParty/dcmtk/include)
set(DCMTKJPEG_LIBRARIES E:/download/ThirdParty/dcmtk/lib)
set(Boost_INCLUDE_DIRS E:/download/ThirdParty/boost_1_65_0/boost)
set(Boost_LIBRARY_DIR E:/download/ThirdParty/boost_1_65_0/lib64-msvc-12.0/lib)
set(JPEG_INCLUDE_DIR E:/download/ThirdParty/jpeg/include)
set(JPEG_LIBRARY E:/download/ThirdParty/jpeg/lib/libjpeg.lib)
# set(JPEG_LIBRARIES D:/qt/5.9/msvc2013_64/lib)
set(JASPER_LIBRARY E:/download/ThirdParty/jasper/lib/libjasper.lib)
set(JASPER_INCLUDE_DIR E:/download/ThirdParty/jasper/include)
set(ZLIB_LIBRARY E:/download/ThirdParty/z/lib/zlib.lib)
set(ZLIB_INCLUDE_DIR E:/download/ThirdParty/z/include)
set(TIFF_LIBRARY E:/download/ThirdParty/tiff/lib/libtiff.lib)
set(TIFF_INCLUDE_DIR E:/download/ThirdParty/tiff/include)
set(PugiXML_LIBRARY E:/download/ThirdParty/pugixml/lib/pugixml.lib)
set(PugiXML_INCLUDE_DIR E:/download/ThirdParty/pugixml/include)
set(OPENSLIDE_LIBRARY E:/download/ThirdParty/openslide/lib/libopenslide.lib)
set(OPENSLIDE_INCLUDE_DIR E:/download/ThirdParty/openslide/include/openslide)
set(OpenCV_LIBS E:/download/opencv/build/x64/vc12/lib)
set(OpenCV_INCLUDE_DIRS E:/download/opencv/build/include)
set(UNITTEST_LIBRARY E:/download/ThirdParty/unittest/lib)
set(UNITTEST_INCLUDE_DIR E:/download/ThirdParty/unittest/include)
#set(SWIG_EXECUTABLE E:/download/ThirdParty/swigwin/swig.exe)
#set(SWIG_DIR E:/download/ThirdParty/swigwin)

CONFIGURE_FILE (
  "${PROJECT_SOURCE_DIR}/config/ASAPMacros.h.in"
  "${PROJECT_BINARY_DIR}/config/ASAPMacros.h"
  )

SET(CMAKE_MODULE_PATH ${DIAGPathology_SOURCE_DIR}/cmakemodules)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

IF(NOT WIN32)
SET(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib:$ORIGIN/")
ENDIF(NOT WIN32)

# Use boost for cross-platform use of threading, file system and date_time handling
FIND_PACKAGE(Boost REQUIRED COMPONENTS date_time filesystem program_options regex system thread)
SET(Boost_DEFINITIONS "-DBOOST_ALL_DYN_LINK")

# Jasper for JPEG2000 codec
FIND_PACKAGE(Jasper REQUIRED)

# TIFF is the main multi-tiled format used in this package
FIND_PACKAGE(TIFF REQUIRED)

# JPEG for the codecs
FIND_PACKAGE(JPEG REQUIRED)

# ZLIB for codec support
FIND_PACKAGE(ZLIB REQUIRED)

# PugiXML required for LIF file format and reading/writing annotations
FIND_PACKAGE(PugiXML REQUIRED)


ADD_LIBRARY(libtiff STATIC IMPORTED)
SET_PROPERTY(TARGET libtiff PROPERTY IMPORTED_LOCATION ${TIFF_LIBRARY})

ADD_LIBRARY(libjasper STATIC IMPORTED)
SET_PROPERTY(TARGET libjasper PROPERTY IMPORTED_LOCATION ${JASPER_LIBRARY})

ADD_LIBRARY(libjpeg STATIC IMPORTED)
SET_PROPERTY(TARGET libjpeg PROPERTY IMPORTED_LOCATION ${JPEG_LIBRARY})

ADD_LIBRARY(zlib STATIC IMPORTED)
SET_PROPERTY(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY})

ADD_LIBRARY(pugixml STATIC IMPORTED)
SET_PROPERTY(TARGET pugixml PROPERTY IMPORTED_LOCATION ${PugiXML_LIBRARY})

# Required for additional format support
FIND_PACKAGE(OPENSLIDE REQUIRED)

# Required for lossless JPEG compression used in for example VSIs
FIND_PACKAGE(DCMTKJPEG REQUIRED)

# Build options for the pathology projects
OPTION(BUILD_DIAG_PATHOLOGY_EXECUTABLES "Builds executables" OFF)
OPTION(BUILD_DIAG_PATHOLOGY_WORKSTATION "Builds workstation" OFF)
OPTION(BUILD_DIAG_PATHOLOGY_IMAGEPROCESSING "Builds image processing routines" OFF)
OPTION(BUILD_DIAG_PATHOLOGY_TESTS "Builds tests" OFF)
OPTION(PACKAGE_ON_INSTALL "Copies dependent DLLs and packages install" OFF)

ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(io)
ADD_SUBDIRECTORY(annotation)


IF(BUILD_DIAG_PATHOLOGY_EXECUTABLES)
  ADD_SUBDIRECTORY(executables)
ENDIF()

IF(BUILD_DIAG_PATHOLOGY_MEVISLAB)
  ADD_SUBDIRECTORY(mevislab)
ENDIF()

IF(BUILD_DIAG_PATHOLOGY_IMAGEPROCESSING)
  ADD_SUBDIRECTORY(imgproc)
ENDIF()

IF(BUILD_DIAG_PATHOLOGY_WORKSTATION)
  ADD_SUBDIRECTORY(ASAP)
ENDIF()

ADD_SUBDIRECTORY(buildtools)

set(INSTALL_CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/cmake)
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(INSTALL_BINARY_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/lib)

# Create the FooBarConfig.cmake and FooBarConfigVersion files
file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}" "${INSTALL_INCLUDE_DIR}")
file(RELATIVE_PATH REL_BINARY_DIR "${INSTALL_CMAKE_DIR}" "${INSTALL_BINARY_DIR}")
file(RELATIVE_PATH REL_LIBRARY_DIR "${INSTALL_CMAKE_DIR}" "${INSTALL_LIBRARY_DIR}")

configure_file(ASAPConfig.cmake.in
  "${PROJECT_BINARY_DIR}/ASAPConfig.cmake" @ONLY)
configure_file(ASAPConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/ASAPConfigVersion.cmake" @ONLY)
 
# Install the FooBarConfig.cmake and FooBarConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/ASAPConfig.cmake"
  "${PROJECT_BINARY_DIR}/ASAPConfigVersion.cmake"
  DESTINATION cmake)
 
# Install the export set for use with the install-tree
install(EXPORT asap-targets DESTINATION cmake)

IF(PACKAGE_ON_INSTALL)
set(CPACK_PACKAGE_NAME "ASAP")
set(CPACK_PACKAGE_VENDOR "Diagnostic Image Analysis Group")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ASAP - Automated Slide Analysis Platform")
set(CPACK_PACKAGE_DESCRIPTION "This package provides viewing and analysis tools for whole slide images.")
set(CPACK_PACKAGE_VERSION "${CURRENT_MAJOR_VERSION}.${CURRENT_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${CURRENT_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${CURRENT_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${CURRENT_PATCH_VERSION})
IF(WIN32)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
SET(CPACK_NSIS_DISPLAY_NAME "ASAP ${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES asap "ASAP")
set(CPACK_NSIS_MUI_ICON ${DIAGPathology_SOURCE_DIR}/ASAP/application.ico)
set(CPACK_NSIS_MUI_UNIICON ${DIAGPathology_SOURCE_DIR}/ASAP/application.ico)
ELSE(WIN32)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Geert Litjens") 
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "ASAP - Automated Slide Analysis Platform")
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/ASAP)
ENDIF(WIN32)
INCLUDE(CPack)
ENDIF(PACKAGE_ON_INSTALL)
