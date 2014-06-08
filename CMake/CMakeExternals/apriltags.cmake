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
# apriltags - external project for tracking AR markers.
#-----------------------------------------------------------------------------

# Sanity checks
if(DEFINED apriltags_DIR AND NOT EXISTS ${apriltags_DIR})
  message(FATAL_ERROR "apriltags_DIR variable is defined but corresponds to non-existing directory \"${apriltags_DIR}\".")
endif()

if(BUILD_IGI)

  set(proj apriltags)
  set(proj_DEPENDENCIES OpenCV Eigen)
  set(apriltags_DEPENDS ${proj})
  set(proj_INSTALL ${CMAKE_BINARY_DIR}/${proj}-install)
 
  if(NOT DEFINED apriltags_DIR)

    if(UNIX)
      set(APRILTAGS_CXX_FLAGS "-fPIC")
      set(APRILTAGS_C_FLAGS "-fPIC")
    endif()
  
    niftkMacroGetChecksum(NIFTK_CHECKSUM_APRILTAGS ${NIFTK_LOCATION_APRILTAGS})
  
    ExternalProject_Add(${proj}
      SOURCE_DIR ${proj}-src
      BINARY_DIR ${proj}-build
      PREFIX ${proj}-cmake
      INSTALL_DIR ${proj}-install
      URL ${NIFTK_LOCATION_APRILTAGS}
      URL_MD5 ${NIFTK_CHECKSUM_APRILTAGS}
      UPDATE_COMMAND ${GIT_EXECUTABLE} checkout ${NIFTK_VERSION_APRILTAGS}
      CMAKE_GENERATOR ${GEN}
      CMAKE_ARGS
          ${EP_COMMON_ARGS}
          -DBUILD_SHARED_LIBS:BOOL=OFF
          -DCMAKE_INSTALL_PREFIX:PATH=${proj_INSTALL}
          -DOpenCV_DIR:PATH=${CMAKE_BINARY_DIR}/OpenCV-build
          -DEigen_DIR:PATH=${Eigen_DIR}
          "-DCMAKE_CXX_FLAGS:STRING=${EP_COMMON_CXX_FLAGS} ${APRILTAGS_CXX_FLAGS}"
          "-DCMAKE_C_FLAGS:STRING=${EP_COMMON_C_FLAGS} ${APRILTAGS_C_FLAGS}"
       DEPENDS ${proj_DEPENDENCIES}
      )

    set(apriltags_DIR ${proj_INSTALL})
    message("SuperBuild loading AprilTags from ${apriltags_DIR}")
 
  else(NOT DEFINED apriltags_DIR)
  
    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
  
  endif(NOT DEFINED apriltags_DIR)

endif()

