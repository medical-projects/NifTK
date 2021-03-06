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

add_subdirectory(Exceptions)

set(niftkcommon_SRCS
  niftkCommandLineParser.cxx
  niftkConversionUtils.cxx
  niftkCSVRow.cxx
  niftkEnvironmentHelper.cxx
  niftkFileHelper.cxx
  niftkLogHelper.cxx
  niftkMathsUtils.cxx
)

add_library(niftkcommon ${niftkcommon_SRCS})

if(BUILD_SHARED_LIBS)
  if(WIN32)
    add_definitions(-DNIFTKCOMMON_WINDOWS_EXPORT=1)
  endif(WIN32)
endif(BUILD_SHARED_LIBS)

target_link_libraries(niftkcommon
  PUBLIC ${ITK_LIBRARIES}
  PRIVATE ${Boost_LIBRARIES}
)

if (NIFTK_USE_COTIRE AND COMMAND cotire)
  cotire(niftkcommon)
endif()

MITK_INSTALL(TARGETS niftkcommon)

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
