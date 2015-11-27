/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkIGIDataSourceManager.h"

#include <usGetModuleContext.h>

#include <mitkExceptionMacro.h>

#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QMessageBox>

namespace niftk
{

const int   IGIDataSourceManager::DEFAULT_FRAME_RATE = 20;
const char* IGIDataSourceManager::DEFAULT_RECORDINGDESTINATION_ENVIRONMENTVARIABLE = "NIFTK_IGIDATASOURCES_DEFAULTRECORDINGDESTINATION";

//-----------------------------------------------------------------------------
IGIDataSourceManager::IGIDataSourceManager(mitk::DataStorage::Pointer dataStorage)
: m_DataStorage(dataStorage)
, m_SetupGuiHasBeenCalled(false)
, m_GuiUpdateTimer(NULL)
, m_FrameRate(DEFAULT_FRAME_RATE)
{
  if (m_DataStorage.IsNull())
  {
    mitkThrow() << "Data Storage is NULL!";
  }
  m_DirectoryPrefix = this->GetDefaultPath();
}


//-----------------------------------------------------------------------------
IGIDataSourceManager::~IGIDataSourceManager()
{
  if (m_SetupGuiHasBeenCalled)
  {
    if (m_GuiUpdateTimer != NULL)
    {
      m_GuiUpdateTimer->stop();
    }

    bool ok = false;
    ok = QObject::disconnect(m_AddSourcePushButton, SIGNAL(clicked()), this, SLOT(OnAddSource()) );
    assert(ok);
    ok = QObject::disconnect(m_GuiUpdateTimer, SIGNAL(timeout()), this, SLOT(OnUpdateGui()));
    assert(ok);
    ok = QObject::disconnect(m_RemoveSourcePushButton, SIGNAL(clicked()), this, SLOT(OnRemoveSource()) );
    assert(ok);
  }
}


//-----------------------------------------------------------------------------
QString IGIDataSourceManager::GetDefaultPath()
{
  QString path;
  QDir directory;

  // if the user has configured a per-machine default location for igi data.
  // if that path exist we use it as a default (prefs from uk_ac_ucl_cmic_igidatasources will override it if necessary).
  QProcessEnvironment   myEnv = QProcessEnvironment::systemEnvironment();
  path = myEnv.value(DEFAULT_RECORDINGDESTINATION_ENVIRONMENTVARIABLE, "");
  directory.setPath(path);

  if (!directory.exists())
  {
    path = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    directory.setPath(path);
  }
  if (!directory.exists())
  {
    path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    directory.setPath(path);
  }
  if (!directory.exists())
  {
    path = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    directory.setPath(path);
  }
  if (!directory.exists())
  {
    path = QDir::currentPath();
    directory.setPath(path);
  }
  if (!directory.exists())
  {
    path = "";
  }
  return path;
}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::setupUi(QWidget* parent)
{
  Ui_IGIDataSourceManager::setupUi(parent);

  m_PlayPushButton->setIcon(QIcon(":/niftkIGIDataSourcesManagerResources/play.png"));
  m_RecordPushButton->setIcon(QIcon(":/niftkIGIDataSourcesManagerResources/record.png"));
  m_StopPushButton->setIcon(QIcon(":/niftkIGIDataSourcesManagerResources/stop.png"));

  m_PlayPushButton->setEnabled(true);
  m_RecordPushButton->setEnabled(true);
  m_StopPushButton->setEnabled(false);

  // Lookup All Available IGIDataSourceFactoryServices.
  m_ModuleContext = us::GetModuleContext();
  if (m_ModuleContext == NULL)
  {
    mitkThrow() << "Unable to get us::ModuleContext!";
  }

  m_Refs = m_ModuleContext->GetServiceReferences<IGIDataSourceFactoryServiceI>();
  if (m_Refs.size() == 0)
  {
    mitkThrow() << "Unable to get us::ServiceReferences for IGIDataSourceFactoryServices!";
  }

  // Iterate through all available factories to populate the combo box.
  for (int i = 0; i < m_Refs.size(); i++)
  {
    niftk::IGIDataSourceFactoryServiceI *factory = m_ModuleContext->GetService<niftk::IGIDataSourceFactoryServiceI>(m_Refs[i]);
    QString name = QString::fromStdString(factory->GetDisplayName());
    m_NameToFactoriesMap.insert(name, factory);
    m_SourceSelectComboBox->addItem(name);
  }
  if (m_Refs.size() != m_NameToFactoriesMap.size())
  {
    mitkThrow() << "Found " << m_Refs.size() << " and " << m_NameToFactoriesMap.size() << " uniquely named IGIDataSourceFactoryServices. These numbers should match.";
  }

  m_GuiUpdateTimer = new QTimer(this);
  m_GuiUpdateTimer->setInterval(1000/(int)(DEFAULT_FRAME_RATE));

  bool ok = false;
  ok = QObject::connect(m_AddSourcePushButton, SIGNAL(clicked()), this, SLOT(OnAddSource()) );
  assert(ok);
  ok = QObject::connect(m_GuiUpdateTimer, SIGNAL(timeout()), this, SLOT(OnUpdateGui()));
  assert(ok);
  ok = QObject::connect(m_RemoveSourcePushButton, SIGNAL(clicked()), this, SLOT(OnRemoveSource()) );
  assert(ok);

  m_SetupGuiHasBeenCalled = true;
}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::OnAddSource()
{
  QString name = m_SourceSelectComboBox->currentText();
  if (!m_NameToFactoriesMap.contains(name))
  {
    mitkThrow() << "Cannot find a factory for " << name.toStdString();
  }

  niftk::IGIDataSourceFactoryServiceI *factory = m_NameToFactoriesMap[name];
  if (factory == NULL)
  {
    mitkThrow() << "Failed to retrieve factory for " << name.toStdString();
  }

  // First create data source.
  niftk::IGIDataSourceI::Pointer source = factory->Create(m_DataStorage);
  if (source.IsNull())
  {
    mitkThrow() << "Failed to create data source for " << name.toStdString();
  }
  m_Sources.push_back(source);

  if (factory->GetNeedGuiAtStartup())
  {
    // Catch All Exceptions.
    try
    {
    } catch (mitk::Exception& e)
    {
      QMessageBox::critical(this, QString("Creating ") + name,
        QString("Unknown ERROR:") + QString(e.GetDescription()),
        QMessageBox::Ok);
    }
  }

  // Launch timers
  if (!m_GuiUpdateTimer->isActive())
  {
    m_GuiUpdateTimer->start();
  }
}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::OnRemoveSource()
{
  if (m_TableWidget->rowCount() == 0)
  {
    return;
  }

  // Stop the timers to make sure they don't trigger.
  bool guiTimerWasOn = m_GuiUpdateTimer->isActive();
  m_GuiUpdateTimer->stop();

  // Get a valid row number, or delete the last item in the table.
  int rowIndex = m_TableWidget->currentRow();
  if (rowIndex < 0)
  {
    rowIndex = m_TableWidget->rowCount() - 1;
  }

  // Given we stopped the timers to make sure they don't trigger, we need
  // to restart them, if indeed they were on.
  if (m_TableWidget->rowCount() > 0)
  {
    if (guiTimerWasOn)
    {
      m_GuiUpdateTimer->start();
    }
  }
}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::OnUpdateGui()
{

}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::StartRecording()
{

}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::StopRecording()
{

}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::SetDirectoryPrefix(const QString& directoryPrefix)
{
  m_DirectoryPrefix = directoryPrefix;
  this->Modified();
}


//-----------------------------------------------------------------------------
void IGIDataSourceManager::SetFramesPerSecond(const int& framesPerSecond)
{
  if (m_GuiUpdateTimer != NULL)
  {
    int milliseconds = 1000 / framesPerSecond;
    m_GuiUpdateTimer->setInterval(milliseconds);
  }

  m_FrameRate = framesPerSecond;
  this->Modified();
}


} // end namespace
