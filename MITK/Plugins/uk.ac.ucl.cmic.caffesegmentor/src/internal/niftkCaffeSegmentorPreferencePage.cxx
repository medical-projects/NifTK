/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkCaffeSegmentorPreferencePage.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>

#include <berryIPreferencesService.h>
#include <berryPlatform.h>
#include <ctkPathLineEdit.h>

#include <niftkBaseView.h>

namespace niftk
{

const QString CaffeSegmentorPreferencePage::PREFERENCES_NODE_NAME("/uk_ac_ucl_cmic_caffesegmentor");
const QString CaffeSegmentorPreferencePage::NETWORK_DESCRIPTION_FILE_NAME("network description");
const QString CaffeSegmentorPreferencePage::NETWORK_WEIGHTS_FILE_NAME("network weights");
const QString CaffeSegmentorPreferencePage::DO_TRANSPOSE_NAME("do transpose");
const QString CaffeSegmentorPreferencePage::INPUT_LAYER_NAME("input layer");
const QString CaffeSegmentorPreferencePage::OUTPUT_BLOB_NAME("output blob");
const QString CaffeSegmentorPreferencePage::GPU_DEVICE_NAME("GPU device");

//-----------------------------------------------------------------------------
CaffeSegmentorPreferencePage::CaffeSegmentorPreferencePage()
: m_MainControl(0)
, m_NetworkDescriptionFileName(nullptr)
, m_NetworkWeightsFileName(nullptr)
, m_DoTranspose(nullptr)
, m_NameMemoryLayer(nullptr)
, m_NameOutputBlob(nullptr)
, m_GPUDevice(nullptr)
, m_Initializing(false)
{

}


//-----------------------------------------------------------------------------
CaffeSegmentorPreferencePage::CaffeSegmentorPreferencePage(const CaffeSegmentorPreferencePage& other)
: berry::Object(), QObject()
{
  Q_UNUSED(other)
  throw std::runtime_error("Copy constructor not implemented");
}


//-----------------------------------------------------------------------------
CaffeSegmentorPreferencePage::~CaffeSegmentorPreferencePage()
{
}


//-----------------------------------------------------------------------------
void CaffeSegmentorPreferencePage::Init(berry::IWorkbench::Pointer )
{

}


//-----------------------------------------------------------------------------
void CaffeSegmentorPreferencePage::CreateQtControl(QWidget* parent)
{
  m_Initializing = true;

  berry::IPreferencesService* prefService = berry::Platform::GetPreferencesService();
  m_CaffeSegmentorPreferencesNode = prefService->GetSystemPreferences()->Node(PREFERENCES_NODE_NAME);

  m_MainControl = new QWidget(parent);

  QFormLayout *formLayout = new QFormLayout;
  m_MainControl->setLayout(formLayout);

  m_NetworkDescriptionFileName = new ctkPathLineEdit();
  formLayout->addRow("network description file name", m_NetworkDescriptionFileName);

  m_NetworkWeightsFileName = new ctkPathLineEdit();
  formLayout->addRow("network weights file name", m_NetworkWeightsFileName);

  m_DoTranspose = new QCheckBox();
  formLayout->addRow("transpose input/output data", m_DoTranspose);

  m_NameMemoryLayer = new QLineEdit();
  formLayout->addRow("input layer name", m_NameMemoryLayer);

  m_NameOutputBlob = new QLineEdit();
  formLayout->addRow("output blob name", m_NameOutputBlob);

  m_GPUDevice = new QSpinBox();
  m_GPUDevice->setMinimum(-1);
  m_GPUDevice->setMaximum(10);
  m_GPUDevice->setSingleStep(1);
  formLayout->addRow("GPU device", m_GPUDevice);

  this->Update();

  m_Initializing = false;
}


//-----------------------------------------------------------------------------
QWidget* CaffeSegmentorPreferencePage::GetQtControl() const
{
  return m_MainControl;
}


//-----------------------------------------------------------------------------
bool CaffeSegmentorPreferencePage::PerformOk()
{
  m_CaffeSegmentorPreferencesNode->Put(CaffeSegmentorPreferencePage::NETWORK_DESCRIPTION_FILE_NAME, m_NetworkDescriptionFileName->currentPath());
  m_CaffeSegmentorPreferencesNode->Put(CaffeSegmentorPreferencePage::NETWORK_WEIGHTS_FILE_NAME, m_NetworkWeightsFileName->currentPath());
  m_CaffeSegmentorPreferencesNode->PutBool(CaffeSegmentorPreferencePage::DO_TRANSPOSE_NAME, m_DoTranspose->isChecked());
  m_CaffeSegmentorPreferencesNode->Put(CaffeSegmentorPreferencePage::INPUT_LAYER_NAME, m_NameMemoryLayer->text());
  m_CaffeSegmentorPreferencesNode->Put(CaffeSegmentorPreferencePage::OUTPUT_BLOB_NAME, m_NameOutputBlob->text());
  m_CaffeSegmentorPreferencesNode->PutInt(CaffeSegmentorPreferencePage::GPU_DEVICE_NAME, m_GPUDevice->value());
  return true;
}


//-----------------------------------------------------------------------------
void CaffeSegmentorPreferencePage::PerformCancel()
{
}


//-----------------------------------------------------------------------------
void CaffeSegmentorPreferencePage::Update()
{
  m_NetworkDescriptionFileName->setCurrentPath(m_CaffeSegmentorPreferencesNode->Get(CaffeSegmentorPreferencePage::NETWORK_DESCRIPTION_FILE_NAME, ""));
  m_NetworkWeightsFileName->setCurrentPath(m_CaffeSegmentorPreferencesNode->Get(CaffeSegmentorPreferencePage::NETWORK_WEIGHTS_FILE_NAME, ""));
  m_DoTranspose->setChecked(m_CaffeSegmentorPreferencesNode->GetBool(CaffeSegmentorPreferencePage::DO_TRANSPOSE_NAME, true));
  m_NameMemoryLayer->setText(m_CaffeSegmentorPreferencesNode->Get(CaffeSegmentorPreferencePage::INPUT_LAYER_NAME, "data"));
  m_NameOutputBlob->setText(m_CaffeSegmentorPreferencesNode->Get(CaffeSegmentorPreferencePage::OUTPUT_BLOB_NAME, "prediction"));
  m_GPUDevice->setValue(m_CaffeSegmentorPreferencesNode->GetInt(CaffeSegmentorPreferencePage::GPU_DEVICE_NAME, -1));
}

} // end namespace
