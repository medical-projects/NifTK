/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkAtracsysDataSourceService.h"
#include <niftkIGISingleFileBackend.h>
#include <niftkAtracsysTracker.h>
#include <mitkExceptionMacro.h>

namespace niftk
{

//-----------------------------------------------------------------------------
AtracsysDataSourceService::AtracsysDataSourceService(
  QString factoryName,
  const IGIDataSourceProperties& properties,
  mitk::DataStorage::Pointer dataStorage)
: IGITrackerDataSourceService("Atracsys", factoryName, dataStorage)
, m_DataGrabbingThread(nullptr)
{
  if(!properties.contains("file"))
  {
    mitkThrow() << "Config file name not specified!";
  }
  QString fileName = (properties.value("file")).toString();

  // First assign to base class (protected) pointers.
  m_Tracker = niftk::AtracsysTracker::New(dataStorage, fileName.toStdString());
  m_BackEnd = niftk::IGISingleFileBackend::New(this->GetName(), this->GetDataStorage());
  m_BackEnd->SetExpectedFramesPerSecond(m_Tracker->GetExpectedFramesPerSecond());

  this->SetTimeStampTolerance((m_Tracker->GetExpectedFramesPerSecond()
                               / m_Tracker->GetExpectedNumberOfTools())
                              *1000000 // convert to nanoseconds
                              *5       // allow up to 5 frames late
                             );

  this->SetProperties(properties);
  this->SetShouldUpdate(true);

  m_DataGrabbingThread = new niftk::IGIDataSourceGrabbingThread(NULL, this);

  bool isOk = connect(m_DataGrabbingThread, SIGNAL(ErrorOccured(QString)), this, SLOT(OnErrorFromThread(QString)));
  assert(isOk);

  m_DataGrabbingThread->SetInterval(1000 / m_Tracker->GetExpectedFramesPerSecond());
  m_DataGrabbingThread->start();

  if (!m_DataGrabbingThread->isRunning())
  {
    mitkThrow() << "Failed to start data grabbing thread";
  }

  this->SetDescription(this->GetName());
  this->SetStatus("Initialised");
  this->Modified();
}


//-----------------------------------------------------------------------------
AtracsysDataSourceService::~AtracsysDataSourceService()
{
  m_DataGrabbingThread->ForciblyStop();

  bool isOk = disconnect(m_DataGrabbingThread, SIGNAL(ErrorOccured(QString)), this, SLOT(OnErrorFromThread(QString)));
  assert(isOk);

  delete m_DataGrabbingThread;
}


//-----------------------------------------------------------------------------
void AtracsysDataSourceService::OnErrorFromThread(QString message)
{
  this->OnErrorOccurred(message);
}

} // end namespace
