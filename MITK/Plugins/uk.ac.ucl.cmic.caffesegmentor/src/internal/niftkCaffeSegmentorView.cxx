/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkCaffeSegmentorView.h"
#include "niftkCaffeSegmentorPreferencePage.h"
#include "niftkCaffeSegmentorActivator.h"
#include <niftkCaffeSegController.h>

#include <berryPlatform.h>
#include <berryIBerryPreferences.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <ctkDictionary.h>
#include <ctkPluginContext.h>
#include <ctkServiceReference.h>
#include <service/event/ctkEventConstants.h>
#include <service/event/ctkEventAdmin.h>
#include <service/event/ctkEvent.h>

namespace niftk
{

const std::string CaffeSegmentorView::VIEW_ID = "uk.ac.ucl.cmic.caffesegmentor";

//-----------------------------------------------------------------------------
CaffeSegmentorView::CaffeSegmentorView()
: BaseView()
{
}


//-----------------------------------------------------------------------------
CaffeSegmentorView::CaffeSegmentorView(const CaffeSegmentorView& other)
{
  Q_UNUSED(other)
  throw std::runtime_error("Copy constructor not implemented");
}


//-----------------------------------------------------------------------------
CaffeSegmentorView::~CaffeSegmentorView()
{
  if (m_CaffeSegController != nullptr)
  {
    delete m_CaffeSegController;
  }
}


//-----------------------------------------------------------------------------
std::string CaffeSegmentorView::GetViewID() const
{
  return VIEW_ID;
}


//-----------------------------------------------------------------------------
void CaffeSegmentorView::SetFocus()
{
  // it seems best not to force the focus, and just leave the
  // focus with whatever the user pressed ... i.e. let Qt handle it.
}

//-----------------------------------------------------------------------------
void CaffeSegmentorView::CreateQtPartControl(QWidget* parent)
{
  this->SetParent(parent);

  m_CaffeSegController = new CaffeSegController(this);
  m_CaffeSegController->SetupGUI(parent);

  // Retrieving preferences done in another method so we can call it on startup, and when prefs change.
  this->RetrievePreferenceValues();

  ctkServiceReference ref = niftk::CaffeSegmentorActivator::getContext()->getServiceReference<ctkEventAdmin>();
  if (ref)
  {
    ctkEventAdmin* eventAdmin = niftk::CaffeSegmentorActivator::getContext()->getService<ctkEventAdmin>(ref);
    ctkDictionary properties;
    properties[ctkEventConstants::EVENT_TOPIC] = "uk/ac/ucl/cmic/IGIUPDATE";
    eventAdmin->subscribeSlot(this, SLOT(OnUpdate(ctkEvent)), properties);
  }
}


//-----------------------------------------------------------------------------
void CaffeSegmentorView::OnPreferencesChanged(const berry::IBerryPreferences*)
{
  this->RetrievePreferenceValues();
}


//-----------------------------------------------------------------------------
void CaffeSegmentorView::RetrievePreferenceValues()
{
  berry::IPreferencesService* prefService = berry::Platform::GetPreferencesService();
  assert( prefService );

  berry::IBerryPreferences::Pointer prefs
      = (prefService->GetSystemPreferences()->Node(CaffeSegmentorPreferencePage::PREFERENCES_NODE_NAME))
        .Cast<berry::IBerryPreferences>();
  assert( prefs );

  std::string networkDescription = prefs->Get(CaffeSegmentorPreferencePage::NETWORK_DESCRIPTION_FILE_NAME, "").toStdString();
  m_CaffeSegController->SetNetworkDescriptionFileName(networkDescription);

  std::string networkWeights = prefs->Get(CaffeSegmentorPreferencePage::NETWORK_WEIGHTS_FILE_NAME, "").toStdString();
  m_CaffeSegController->SetNetworkWeightsFileName(networkWeights);
}


//-----------------------------------------------------------------------------
void CaffeSegmentorView::OnUpdate(const ctkEvent& event)
{
  m_CaffeSegController->Update();
}

} // end namespace
