#/*============================================================================
#
#  NifTK: A software platform for medical image computing.
#
#  Copyright (c) University College London (UCL). All rights reserved.
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  See LICENSE.txt in the top level directory for details.
#
#============================================================================*/


#-----------------------------------------------------------------------------
# VL
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED VL_ROOT AND NOT EXISTS ${VL_ROOT})
  message(FATAL_ERROR "VL_ROOT variable is defined but corresponds to non-existing directory \"${VL_ROOT}\".")
endif()

if(BUILD_VL)

  set(version "bcc082625d")
  set(location "https://cmiclab.cs.ucl.ac.uk/CMIC/VisualizationLibrary.git")
  set(depends "")

  niftkMacroDefineExternalProjectVariables(VL ${version} ${location} "${depends}")

  if(NOT DEFINED VL_DIR)

    # Note:
    # The VL_ROOT variable has to be defined up here because the
    # ChangeVLLibsInstallNameForMac.cmake.in script refers to it.

    set(VL_ROOT ${proj_INSTALL})

    set(_test_options )
    if(APPLE)
      set(APPLE_CMAKE_SCRIPT ${proj_CONFIG}/ChangeVLLibsInstallNameForMac.cmake)
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/ExternalProjects/ChangeVLLibsInstallNameForMac.cmake.in ${APPLE_CMAKE_SCRIPT} @ONLY)
      set(APPLE_TEST_COMMAND ${CMAKE_COMMAND} -P ${APPLE_CMAKE_SCRIPT})
      set(_test_options
        TEST_AFTER_INSTALL 1
        TEST_COMMAND ${APPLE_TEST_COMMAND}
      )
    endif()

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ^^
      PREFIX ${proj_CONFIG}
      SOURCE_DIR ${proj_SOURCE}
      BINARY_DIR ${proj_BUILD}
      INSTALL_DIR ${proj_INSTALL}
      GIT_REPOSITORY ${proj_LOCATION}
      GIT_TAG ${proj_VERSION}
      UPDATE_COMMAND ${GIT_EXECUTABLE} checkout ${proj_VERSION}
      ${_test_options}
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
        ${EP_COMMON_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${NifTK_PREFIX_PATH}
        -DBUILD_SHARED_LIBS:BOOL=${EP_BUILD_SHARED_LIBS}
        -DBUILD_TESTING:BOOL=${EP_BUILD_TESTING}
        -DCMAKE_INSTALL_PREFIX:PATH=${proj_INSTALL}
        -DVL_GUI_QT5_SUPPORT:BOOL=${MITK_USE_Qt5}
        -DVL_GUI_QT5_EXAMPLES:BOOL=0
        -DVL_GUI_QT4_SUPPORT:BOOL=${MITK_USE_Qt4}
        -DVL_GUI_QT4_EXAMPLES:BOOL=0
        -DVL_USER_DATA_OBJECT:BOOL=1
        ${additional_cmake_args}
      CMAKE_CACHE_ARGS
        ${EP_COMMON_CACHE_ARGS}
      CMAKE_CACHE_DEFAULT_ARGS
        ${EP_COMMON_CACHE_DEFAULT_ARGS}
      DEPENDS ${proj_DEPENDENCIES}
    )

    set(NifTK_PREFIX_PATH ${proj_INSTALL}^^${NifTK_PREFIX_PATH})
    mitkFunctionInstallExternalCMakeProject(${proj})

    message("SuperBuild loading VL from ${VL_ROOT}")

  else()

    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

  endif()
endif()
