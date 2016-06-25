/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkIGIVideoOverlayWidget.h"
#include <mitkCoordinateAxesData.h>
#include <mitkNodePredicateDataType.h>
#include <mitkImage.h>
#include <mitkBaseRenderer.h>
#include <mitkRenderingManager.h>
#include <mitkTimeGeometry.h>
#include <mitkGlobalInteraction.h>
#include <mitkFocusManager.h>

namespace niftk
{

//-----------------------------------------------------------------------------
IGIVideoOverlayWidget::IGIVideoOverlayWidget(QWidget * /*parent*/)
{
  this->setupUi(this);

  m_OpacitySlider->setMinimum(0);
  m_OpacitySlider->setMaximum(100);
  m_OpacitySlider->setSingleStep(1);
  m_OpacitySlider->setPageStep(10);
  m_OpacitySlider->setValue(static_cast<int>(m_LeftOverlayViewer->GetOpacity()*100));

  m_3DViewer->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D );

  connect(m_3DViewCheckBox, SIGNAL(toggled(bool)), this, SLOT(On3DViewerCheckBoxChecked(bool)));
  connect(m_LeftImageCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnLeftOverlayCheckBoxChecked(bool)));
  connect(m_RightImageCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnRightOverlayCheckBoxChecked(bool)));

  connect(m_LeftImageCombo, SIGNAL(OnSelectionChanged(const mitk::DataNode*)), this, SLOT(OnLeftImageSelected(const mitk::DataNode*)));
  connect(m_RightImageCombo, SIGNAL(OnSelectionChanged(const mitk::DataNode*)), this, SLOT(OnRightImageSelected(const mitk::DataNode*)));
  connect(m_TrackingCombo, SIGNAL(OnSelectionChanged(const mitk::DataNode*)), this, SLOT(OnTransformSelected(const mitk::DataNode*)));
  connect(m_OpacitySlider, SIGNAL(sliderMoved(int)), this, SLOT(OnOpacitySliderMoved(int)));

  m_LeftImageCombo->setCurrentIndex(0);
  m_RightImageCombo->setCurrentIndex(0);
  m_TrackingCombo->setCurrentIndex(0);

  m_LeftImageCheckBox->setChecked(true);
  m_RightImageCheckBox->setChecked(true);
  m_3DViewCheckBox->setChecked(true);

  int width = m_Splitter->width();
  QList<int> sizes;
  sizes.append(width);
  sizes.append(width);
  m_Splitter->setSizes(sizes);
  m_Splitter->setChildrenCollapsible(true);
}


//-----------------------------------------------------------------------------
IGIVideoOverlayWidget::~IGIVideoOverlayWidget()
{
  this->DeRegisterDataStorageListeners();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::DeRegisterDataStorageListeners()
{
  if (m_DataStorage.IsNotNull())
  {
    m_DataStorage->ChangedNodeEvent.RemoveListener
      (mitk::MessageDelegate1<IGIVideoOverlayWidget, const mitk::DataNode*>
      (this, &IGIVideoOverlayWidget::NodeChanged ) );
  }
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnLeftOverlayCheckBoxChecked(bool checked)
{
  if (checked)
  {
    mitk::RenderingManager::GetInstance()->AddRenderWindow(m_LeftOverlayViewer->GetRenderWindow()->GetVtkRenderWindow());
  }
  else
  {
    mitk::RenderingManager::GetInstance()->RemoveRenderWindow(m_LeftOverlayViewer->GetRenderWindow()->GetVtkRenderWindow());
  }
  m_LeftOverlayViewer->setVisible(checked);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnRightOverlayCheckBoxChecked(bool checked)
{
  if (checked)
  {
    mitk::RenderingManager::GetInstance()->AddRenderWindow(m_RightOverlayViewer->GetRenderWindow()->GetVtkRenderWindow());
  }
  else
  {
    mitk::RenderingManager::GetInstance()->RemoveRenderWindow(m_RightOverlayViewer->GetRenderWindow()->GetVtkRenderWindow());
  }
  m_RightOverlayViewer->setVisible(checked);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::On3DViewerCheckBoxChecked(bool checked)
{
  if (checked)
  {
    mitk::RenderingManager::GetInstance()->AddRenderWindow(m_3DViewer->GetRenderWindow());
  }
  else
  {
    mitk::RenderingManager::GetInstance()->RemoveRenderWindow(m_3DViewer->GetRenderWindow());
  }
  m_3DViewer->setVisible(checked);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnOpacitySliderMoved(int value)
{
  m_LeftOverlayViewer->SetOpacity(value / 100.0);
  m_RightOverlayViewer->SetOpacity(value / 100.0);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnLeftImageSelected(const mitk::DataNode* node)
{
  m_LeftOverlayViewer->SetImageNode(node);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnRightImageSelected(const mitk::DataNode* node)
{
  m_RightOverlayViewer->SetImageNode(node);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::OnTransformSelected(const mitk::DataNode* node)
{
  m_LeftOverlayViewer->SetTransformNode(node);
  m_RightOverlayViewer->SetTransformNode(node);
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::SetDataStorage(mitk::DataStorage* storage)
{
  if (m_DataStorage.IsNotNull() && m_DataStorage != storage)
  {
    this->DeRegisterDataStorageListeners();
  }

  m_DataStorage = storage;
  
  if (m_DataStorage.IsNotNull())
  {
    m_DataStorage->ChangedNodeEvent.AddListener
      (mitk::MessageDelegate1<IGIVideoOverlayWidget, const mitk::DataNode*>
      (this, &IGIVideoOverlayWidget::NodeChanged ) );
  }
  
  mitk::TimeGeometry::Pointer geometry = storage->ComputeBoundingGeometry3D(storage->GetAll());
  mitk::RenderingManager::GetInstance()->InitializeView(m_3DViewer->GetVtkRenderWindow(), geometry);

  m_3DViewer->GetRenderer()->SetDataStorage(storage);
  m_LeftOverlayViewer->SetDataStorage(storage);
  m_RightOverlayViewer->SetDataStorage(storage);
  m_LeftImageCombo->SetDataStorage(storage);
  m_RightImageCombo->SetDataStorage(storage);
  m_TrackingCombo->SetDataStorage(storage);

  mitk::TNodePredicateDataType<mitk::Image>::Pointer isImage = mitk::TNodePredicateDataType<mitk::Image>::New();
  m_LeftImageCombo->SetPredicate(isImage);
  m_LeftImageCombo->SetAutoSelectNewItems(false);
  m_RightImageCombo->SetPredicate(isImage);
  m_RightImageCombo->SetAutoSelectNewItems(false);

  mitk::TNodePredicateDataType<mitk::CoordinateAxesData>::Pointer isTransform = mitk::TNodePredicateDataType<mitk::CoordinateAxesData>::New();
  m_TrackingCombo->SetPredicate(isTransform);
  m_TrackingCombo->SetAutoSelectNewItems(false);

  m_LeftImageCombo->setCurrentIndex(0);
  this->OnLeftImageSelected(nullptr);
  m_RightImageCombo->setCurrentIndex(0);
  this->OnRightImageSelected(nullptr);
  m_TrackingCombo->setCurrentIndex(0);
  this->OnTransformSelected(nullptr);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::NodeChanged(const mitk::DataNode* node)
{
}


//-----------------------------------------------------------------------------
QmitkRenderWindow* IGIVideoOverlayWidget::GetActiveQmitkRenderWindow() const
{
  QmitkRenderWindow *result = NULL;

  mitk::FocusManager *focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  if (focusManager != NULL)
  {
    mitk::BaseRenderer *renderer = focusManager->GetFocused();
    if (m_LeftOverlayViewer->GetRenderWindow()->GetRenderer() == renderer)
    {
      result = m_LeftOverlayViewer->GetRenderWindow();
    }
    if (m_RightOverlayViewer->GetRenderWindow()->GetRenderer() == renderer)
    {
      result = m_RightOverlayViewer->GetRenderWindow();
    }
    else if (m_3DViewer->GetRenderer() == renderer)
    {
      result = m_3DViewer;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
QHash<QString, QmitkRenderWindow *> IGIVideoOverlayWidget::GetQmitkRenderWindows() const
{
  QHash<QString, QmitkRenderWindow *> result;
  result.insert("left overlay", m_LeftOverlayViewer->GetRenderWindow());
  result.insert("right overlay", m_RightOverlayViewer->GetRenderWindow());
  result.insert("3d", m_3DViewer);
  return result;
}


//-----------------------------------------------------------------------------
QmitkRenderWindow* IGIVideoOverlayWidget::GetQmitkRenderWindow(const QString &id) const
{
  QmitkRenderWindow *result = NULL;
  if (id == "3d")
  {
    result = m_3DViewer;
  }
  else if (id == "left overlay")
  {
    result =  m_LeftOverlayViewer->GetRenderWindow();
  }
  else if (id == "right overlay")
  {
    result =  m_RightOverlayViewer->GetRenderWindow();
  }
  return result;
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::SetDepartmentLogoPath(const QString& path)
{
  m_LeftOverlayViewer->SetDepartmentLogoPath(path);
  m_RightOverlayViewer->SetDepartmentLogoPath(path);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::EnableDepartmentLogo()
{
  m_LeftOverlayViewer->EnableDepartmentLogo();
  m_RightOverlayViewer->EnableDepartmentLogo();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::DisableDepartmentLogo()
{
  m_LeftOverlayViewer->DisableDepartmentLogo();
  m_RightOverlayViewer->DisableDepartmentLogo();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::SetGradientBackgroundColors(const mitk::Color& colour1, const mitk::Color& colour2)
{
  m_LeftOverlayViewer->SetGradientBackgroundColors(colour1, colour2);
  m_RightOverlayViewer->SetGradientBackgroundColors(colour1, colour2);
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::EnableGradientBackground()
{
  m_LeftOverlayViewer->EnableGradientBackground();
  m_RightOverlayViewer->EnableGradientBackground();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::DisableGradientBackground()
{
  m_LeftOverlayViewer->DisableGradientBackground();
  m_RightOverlayViewer->DisableGradientBackground();
}


//-----------------------------------------------------------------------------
void IGIVideoOverlayWidget::Update()
{
  m_LeftOverlayViewer->Update();
  m_RightOverlayViewer->Update();
}

} // end namespace
