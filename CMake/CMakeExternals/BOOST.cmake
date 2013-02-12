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
# BOOST. In SuperBuild we do install it to get include and lib folders
#        in standard place. Which then means that FindBoost will find it.
#-----------------------------------------------------------------------------

# Sanity checks
IF(DEFINED BOOST_ROOT AND NOT EXISTS ${BOOST_ROOT})
  MESSAGE(FATAL_ERROR "BOOST_ROOT variable is defined but corresponds to non-existing directory \"${BOOST_ROOT}\".")
ENDIF()

SET(proj BOOST)
SET(proj_DEPENDENCIES )
SET(proj_INSTALL ${EP_BASE}/Install/${proj})
SET(BOOST_DEPENDS ${proj})

IF(NOT DEFINED BOOST_ROOT)

  SET(additional_cmake_args )

  SET(BOOST_ARGS
    -DENABLE_MULTI_THREADED:BOOL=ON
    -DENABLE_SINGLE_THREADED:BOOL=OFF
    -DWITH_MPI:BOOL=OFF
  )

  IF(${BUILD_SHARED_LIBS})
    IF (BUILD_MESHING)
      IF (NOT WIN32) 
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
      ENDIF (NOT WIN32)
      SET(BOOST_ARGS
        ${BOOST_ARGS}
       -DENABLE_SHARED:BOOL=ON
       -DBUILD_SHARED_LIBS:BOOL=ON
       -DENABLE_STATIC:BOOL=ON
       -DENABLE_STATIC_RUNTIME:BOOL=OFF
      )
    ELSE (BUILD_MESHING)
      SET(BOOST_ARGS
        ${BOOST_ARGS}
        -DENABLE_SHARED:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=ON
        -DENABLE_STATIC:BOOL=OFF
        -DENABLE_STATIC_RUNTIME:BOOL=OFF
      )
    ENDIF (BUILD_MESHING)
  ELSE(${BUILD_SHARED_LIBS})
    IF (NOT WIN32)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    ENDIF (NOT WIN32)
    SET(BOOST_ARGS
      ${BOOST_ARGS}
      -DENABLE_SHARED:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DENABLE_STATIC:BOOL=ON
      -DENABLE_STATIC_RUNTIME:BOOL=ON
      )
  ENDIF(${BUILD_SHARED_LIBS})

  IF(${CMAKE_BUILD_TYPE} MATCHES "Release")
    SET(BOOST_ARGS
      ${BOOST_ARGS}
      -DENABLE_RELEASE:BOOL=ON
      -DENABLE_DEBUG:BOOL=OFF
    )
  ELSE(${CMAKE_BUILD_TYPE} MATCHES "Release")
    SET(BOOST_ARGS
      ${BOOST_ARGS}
      -DENABLE_RELEASE:BOOL=OFF
      -DENABLE_DEBUG:BOOL=ON
    )
  ENDIF(${CMAKE_BUILD_TYPE} MATCHES "Release")

  niftkMacroGetChecksum(NIFTK_CHECKSUM_BOOST ${NIFTK_LOCATION_BOOST})

  ExternalProject_Add(${proj}
    URL ${NIFTK_LOCATION_BOOST}
    URL_MD5 ${NIFTK_CHECKSUM_BOOST}
    CMAKE_GENERATOR ${GEN}
    CMAKE_ARGS
        ${EP_COMMON_ARGS}
        ${BOOST_ARGS}
        -DWITH_BZIP2:BOOL=OFF
        -DWITH_DOXYGEN:BOOL=OFF
        -DWITH_EXPAT:BOOL=OFF
        -DWITH_ICU:BOOL=OFF
        -DWITH_PYTHON:BOOL=OFF
        -DWITH_XSLTPROC:BOOL=OFF
        -DWITH_VALGRIND:BOOL=OFF
        -DWITH_ZLIB:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${EP_BASE}/Install/${proj}
     DEPENDS ${proj_DEPENDENCIES}
    )
  SET(BOOST_ROOT ${proj_INSTALL})
  IF(WIN32)
    SET(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    SET(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
  ELSE(WIN32)
    SET(BOOST_INCLUDEDIR "${BOOST_ROOT}/include/boost-${NIFTK_VERSION_BOOST}")
    SET(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib/boost-${NIFTK_VERSION_BOOST}")
  ENDIF(WIN32)

  MESSAGE("SuperBuild loading Boost from ${BOOST_ROOT}")
  MESSAGE("SuperBuild loading Boost using BOOST_INCLUDEDIR=${BOOST_INCLUDEDIR}")
  MESSAGE("SuperBuild loading Boost using BOOST_LIBRARYDIR=${BOOST_LIBRARYDIR}")

ELSE()

  mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

ENDIF()
