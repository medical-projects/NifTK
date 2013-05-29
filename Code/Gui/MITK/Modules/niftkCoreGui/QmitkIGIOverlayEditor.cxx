/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "QmitkIGIOverlayEditor.h"
#include <mitkNodePredicateDataType.h>
#include <mitkImage.h>
#include <mitkBaseRenderer.h>
#include <mitkRenderingManager.h>
#include <mitkTimeSlicedGeometry.h>
#include <mitkCoordinateAxesData.h>

//-----------------------------------------------------------------------------
QmitkIGIOverlayEditor::QmitkIGIOverlayEditor(QWidget * /*parent*/)
{
  this->setupUi(this);

  m_OpacitySlider->setMinimum(0);
  m_OpacitySlider->setMaximum(100);
  m_OpacitySlider->setSingleStep(1);
  m_OpacitySlider->setPageStep(10);
  m_OpacitySlider->setValue(static_cast<int>(m_OverlayViewer->GetOpacity()*100));

  m_3DViewer->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D );
  m_OverlayViewer->GetRenderWindow1()->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D );

  mitk::RenderingManager::GetInstance()->AddRenderWindow(m_3DViewer->GetVtkRenderWindow());
  mitk::RenderingManager::GetInstance()->AddRenderWindow(m_OverlayViewer->GetRenderWindow1()->GetVtkRenderWindow());

  m_OverlayViewer->setVisible(true);
  m_3DViewer->setVisible(true);

  m_OverlayCheckBox->setChecked(true);
  m_3DViewerCheckBox->setChecked(true);

  connect(m_OverlayCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnOverlayCheckBoxChecked(bool)));
  connect(m_3DViewerCheckBox, SIGNAL(toggled(bool)), this, SLOT(On3DViewerCheckBoxChecked(bool)));
  connect(m_ImageCombo, SIGNAL(OnSelectionChanged(const mitk::DataNode*)), this, SLOT(OnImageSelected(const mitk::DataNode*)));
  connect(m_TransformCombo, SIGNAL(OnSelectionChanged(const mitk::DataNode*)), this, SLOT(OnTransformSelected(const mitk::DataNode*)));

  int width = m_Splitter->width();
  QList<int> sizes;
  sizes.append(width);
  sizes.append(width);
  m_Splitter->setSizes(sizes);
}


//-----------------------------------------------------------------------------
QmitkIGIOverlayEditor::~QmitkIGIOverlayEditor()
{
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::OnOverlayCheckBoxChecked(bool checked)
{
  if (!checked)
  {
    m_3DViewerCheckBox->setEnabled(false);
  }
  else
  {
    m_3DViewerCheckBox->setEnabled(true);
  }
  m_OverlayViewer->setVisible(checked);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::On3DViewerCheckBoxChecked(bool checked)
{
  if (!checked)
  {
    m_OverlayCheckBox->setEnabled(false);
  }
  else
  {
    m_OverlayCheckBox->setEnabled(true);
  }
  m_3DViewer->setVisible(checked);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::OnOpacitySliderMoved(int value)
{
  m_OverlayViewer->SetOpacity(value / 100.0);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::OnImageSelected(const mitk::DataNode* node)
{
  m_OverlayViewer->SetImageNode(node);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::OnTransformSelected(const mitk::DataNode* node)
{
 m_OverlayViewer->SetTransformNode(node);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::SetDataStorage(mitk::DataStorage* storage)
{
  mitk::TNodePredicateDataType<mitk::Image>::Pointer isImage = mitk::TNodePredicateDataType<mitk::Image>::New();
  m_ImageCombo->SetPredicate(isImage);
  m_ImageCombo->SetAutoSelectNewItems(false);

  mitk::TNodePredicateDataType<mitk::CoordinateAxesData>::Pointer isTransform = mitk::TNodePredicateDataType<mitk::CoordinateAxesData>::New();
  m_TransformCombo->SetPredicate(isTransform);
  m_TransformCombo->SetAutoSelectNewItems(false);

  m_ImageCombo->SetDataStorage(storage);
  m_TransformCombo->SetDataStorage(storage);
  m_3DViewer->GetRenderer()->SetDataStorage(storage);
  m_OverlayViewer->SetDataStorage(storage);

  mitk::TimeSlicedGeometry::Pointer geometry = storage->ComputeBoundingGeometry3D(storage->GetAll());
  mitk::RenderingManager::GetInstance()->InitializeView(m_3DViewer->GetVtkRenderWindow(), geometry);
}


//-----------------------------------------------------------------------------
QmitkRenderWindow* QmitkIGIOverlayEditor::GetActiveQmitkRenderWindow() const
{
  // Default implementation for now. Maybe check focus manager to see which one is focussed?
  QmitkRenderWindow *result = m_OverlayViewer->GetRenderWindow1();
  return result;
}


//-----------------------------------------------------------------------------
QHash<QString, QmitkRenderWindow *> QmitkIGIOverlayEditor::GetQmitkRenderWindows() const
{
  QHash<QString, QmitkRenderWindow *> result;
  result.insert("overlay", m_OverlayViewer->GetRenderWindow1());
  result.insert("3d", m_3DViewer);
  return result;
}


//-----------------------------------------------------------------------------
QmitkRenderWindow* QmitkIGIOverlayEditor::GetQmitkRenderWindow(const QString &id) const
{
  QmitkRenderWindow *result = NULL;
  if (id == "3d")
  {
    result = m_3DViewer;
  }
  else if (id == "overlay")
  {
    result =  m_OverlayViewer->GetRenderWindow1();
  }
  return result;
}


//-----------------------------------------------------------------------------
mitk::Point3D QmitkIGIOverlayEditor::GetSelectedPosition(const QString &/*id*/) const
{
  return m_OverlayViewer->GetCrossPosition();
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::SetSelectedPosition(const mitk::Point3D &pos, const QString &/*id*/)
{
  m_OverlayViewer->MoveCrossToPosition(pos);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::SetDepartmentLogoPath(const std::string path)
{
  m_OverlayViewer->SetDepartmentLogoPath(path.c_str());
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::EnableDepartmentLogo()
{
  m_OverlayViewer->EnableDepartmentLogo();
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::DisableDepartmentLogo()
{
  m_OverlayViewer->DisableDepartmentLogo();
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::SetGradientBackgroundColors(const mitk::Color& colour1, const mitk::Color& colour2)
{
  m_OverlayViewer->SetGradientBackgroundColors(colour1, colour2);
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::EnableGradientBackground()
{
  m_OverlayViewer->EnableGradientBackground();
}


//-----------------------------------------------------------------------------
void QmitkIGIOverlayEditor::DisableGradientBackground()
{
  m_OverlayViewer->DisableGradientBackground();
}
