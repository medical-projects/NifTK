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

SET(CPP_FILES
  Common/QmitkIGIUtils.cpp
  DataManagement/QmitkQImageToMitkImageFilter.cpp
  ToolsGui/TrackerControlsWidget.cpp
  ToolsGui/QmitkFiducialRegistrationWidgetDialog.cpp
  ToolsGui/QmitkIGIDataSourceManager.cpp    
  ToolsGui/QmitkIGINiftyLinkDataType.cpp
  ToolsGui/QmitkIGINiftyLinkDataSource.cpp
  ToolsGui/QmitkIGINiftyLinkDataSourceGui.cpp
  ToolsGui/QmitkIGIDataSource.cpp
  ToolsGui/QmitkIGIDataSourceGui.cpp
  ToolsGui/QmitkIGILocalDataSource.cpp
  ToolsGui/QmitkIGITrackerTool.cpp
  ToolsGui/QmitkIGITrackerToolGui.cpp
  ToolsGui/QmitkIGIUltrasonixTool.cpp
  ToolsGui/QmitkIGIUltrasonixToolGui.cpp   
  ToolsGui/QmitkIGIOpenCVDataSource.cpp 
  ToolsGui/QmitkIGIOpenCVDataSourceGui.cpp 
)

SET(MOC_H_FILES
  ToolsGui/TrackerControlsWidget.h
  ToolsGui/QmitkFiducialRegistrationWidgetDialog.h
  ToolsGui/QmitkIGIDataSourceManager.h
  ToolsGui/QmitkIGINiftyLinkDataSource.h
  ToolsGui/QmitkIGINiftyLinkDataSourceGui.h
  ToolsGui/QmitkIGIDataSource.h
  ToolsGui/QmitkIGIDataSourceGui.h
  ToolsGui/QmitkIGILocalDataSource.h
  ToolsGui/QmitkIGITrackerTool.h
  ToolsGui/QmitkIGITrackerToolGui.h  
  ToolsGui/QmitkIGIUltrasonixTool.h
  ToolsGui/QmitkIGIUltrasonixToolGui.h
  ToolsGui/QmitkIGIOpenCVDataSource.h
  ToolsGui/QmitkIGIOpenCVDataSourceGui.h
)

SET(UI_FILES
  ToolsGui/TrackerControlsWidget.ui
  ToolsGui/QmitkFiducialRegistrationWidgetDialog.ui  
  ToolsGui/QmitkIGITrackerToolGui.ui
  ToolsGui/QmitkIGIUltrasonixToolGui.ui
  ToolsGui/QmitkIGIDataSourceManager.ui
)

SET(QRC_FILES
  Resources/niftkIGIGui.qrc
)