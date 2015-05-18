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

set(CPP_FILES
  LookupTables/QmitkLookupTableContainer.cxx
  LookupTables/QmitkLookupTableSaxHandler.cxx
  LookupTables/QmitkLookupTableManager.cxx
  LookupTables/QmitkLookupTableProviderServiceImpl.cxx
  LookupTables/QmitkLookupTableProviderServiceActivator.cxx
  QmitkDataStorageCheckableComboBox.cxx
  QmitkHelpAboutDialog.cxx
  QmitkCmicLogo.cxx
  Rendering/SharedOGLContext.cxx
  Rendering/ScopedOGLContext.cxx
)

set(MOC_H_FILES
  Events/QmitkPaintEventEater.h
  Events/QmitkWheelEventEater.h
  Events/QmitkMouseEventEater.h
  QmitkDataStorageCheckableComboBox.h
  QmitkHelpAboutDialog.h
)

set(UI_FILES
  QmitkHelpAboutDialog.ui
)

set(QRC_FILES
  Resources/niftkCoreGui.qrc
)

if(WIN32)
  set(CPP_FILES
    ${CPP_FILES}
    Events/QmitkWindowsHotkeyHandler.cxx
  )
  set(MOC_H_FILES
    ${MOC_H_FILES}
    Events/QmitkWindowsHotkeyHandler.h
  )
endif()
