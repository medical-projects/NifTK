/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkIGIDataSourceBackgroundSaveThread_h
#define niftkIGIDataSourceBackgroundSaveThread_h

#include "niftkIGIDataSourcesExports.h"
#include "niftkIGITimerBasedThread.h"
#include "niftkIGISaveableDataSourceI.h"

namespace niftk
{

/**
* \class IGIDataSourceBackgroundSaveThread
* \brief Thread class, based on IGITimerBasedThread to simply call "SaveBuffer".
*/
class NIFTKIGIDATASOURCES_EXPORT IGIDataSourceBackgroundSaveThread : public IGITimerBasedThread
{
public:
  IGIDataSourceBackgroundSaveThread(QObject *parent, IGISaveableDataSourceI *source);
  virtual ~IGIDataSourceBackgroundSaveThread();

  /**
  * \see IGITimerBasedThread::OnTimeoutImpl()
  */
  virtual void OnTimeoutImpl() override;

private:
  IGISaveableDataSourceI *m_Source;
};

} // end namespace

#endif
