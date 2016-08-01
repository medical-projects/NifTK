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

set(SRC_CPP_FILES
  niftkSideViewerView.cxx
  niftkSideViewerWidget.cxx
)

set(INTERNAL_CPP_FILES
  niftkPluginActivator.cxx
)

set(UI_FILES
)

set(MOC_H_FILES
  src/internal/niftkPluginActivator.h
  src/niftkSideViewerView.h
  src/niftkSideViewerWidget.h
)

set(CACHED_RESOURCE_FILES
  resources/sideviewer-icon.png
  plugin.xml
)

# todo: add some qt style sheet resources
set(QRC_FILES
  resources/sideviewer.qrc
)

set(CPP_FILES )

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})
