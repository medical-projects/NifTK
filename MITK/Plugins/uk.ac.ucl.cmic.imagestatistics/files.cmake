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
  
)

set(INTERNAL_CPP_FILES
  uk_ac_ucl_cmic_imagestatistics_Activator.cxx
  ImageStatisticsView.cxx
  ImageStatisticsViewPreferencesPage.cxx
)

set(UI_FILES
  src/internal/ImageStatisticsViewControls.ui
)

set(MOC_H_FILES
  src/internal/uk_ac_ucl_cmic_imagestatistics_Activator.h
  src/internal/ImageStatisticsView.h
  src/internal/ImageStatisticsViewPreferencesPage.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/icon.xpm
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES

)

set(CPP_FILES )

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})
