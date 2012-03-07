/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-07-19 12:16:16 +0100 (Tue, 19 Jul 2011) $
 Revision          : $Revision: 6802 $
 Last modified by  : $Author: mjc $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include "QmitkMIDASMultiViewWidget.h"
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QSize>
#include <QSpinBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>
#include <QMessageBox>
#include <QStackedLayout>
#include "mitkFocusManager.h"
#include "mitkGlobalInteraction.h"
#include "mitkTimeSlicedGeometry.h"
#include "QmitkRenderWindow.h"
#include "QmitkMIDASSingleViewWidget.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

QmitkMIDASMultiViewWidget::QmitkMIDASMultiViewWidget(
    QmitkMIDASMultiViewVisibilityManager* visibilityManager,
    mitk::DataStorage::Pointer dataStorage,
    int defaultNumberOfRows,
    int defaultNumberOfColumns,
    QWidget* parent, Qt::WindowFlags f)
: QWidget(parent, f)
, mitk::MIDASKeyPressResponder()
, m_TopLevelLayout(NULL)
, m_LayoutToPutControlsOnTopOfWindows(NULL)
, m_LayoutForStackingRenderWindows(NULL)
, m_LayoutForRenderWindows(NULL)
, m_LayoutForTopControls(NULL)
, m_VisibilityManager(visibilityManager)
, m_FocusManagerObserverTag(0)
, m_SelectedWindow(0)
, m_DefaultNumberOfRows(defaultNumberOfRows)
, m_DefaultNumberOfColumns(defaultNumberOfColumns)
, m_InteractionEnabled(false)
, m_Show3DViewInOrthoMode(false)
, m_Show2DCursors(false)
, m_IsThumbnailMode(false)
, m_IsMIDASSegmentationMode(false)
{
  assert(visibilityManager);
  assert(dataStorage);

  m_DataStorage = dataStorage;

  m_TopLevelLayout = new QHBoxLayout(this);
  m_TopLevelLayout->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_TopLevelLayout"));
  m_TopLevelLayout->setContentsMargins(0, 0, 0, 0);
  m_TopLevelLayout->setSpacing(0);

  m_LayoutToPutControlsOnTopOfWindows = new QVBoxLayout();
  m_LayoutToPutControlsOnTopOfWindows->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutToPutControlsOnTopOfWindows"));
  m_LayoutToPutControlsOnTopOfWindows->setContentsMargins(0, 0, 0, 0);
  m_LayoutToPutControlsOnTopOfWindows->setSpacing(0);

  m_LayoutForStackingRenderWindows = new QStackedLayout();
  m_LayoutForStackingRenderWindows->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForStackingRenderWindows"));
  m_LayoutForStackingRenderWindows->setContentsMargins(0, 0, 0, 0);
  m_LayoutForStackingRenderWindows->setSpacing(0);

  m_LayoutForRenderWindows = new QGridLayout();
  m_LayoutForRenderWindows->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForRenderWindows"));
  m_LayoutForRenderWindows->setContentsMargins(0, 0, 0, 0);
  m_LayoutForRenderWindows->setVerticalSpacing(0);
  m_LayoutForRenderWindows->setHorizontalSpacing(0);

  m_LayoutForTopControls = new QGridLayout();
  m_LayoutForTopControls->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForTopControls"));
  m_LayoutForTopControls->setContentsMargins(0, 0, 0, 0);
  m_LayoutForTopControls->setVerticalSpacing(0);
  m_LayoutForTopControls->setHorizontalSpacing(0);

  m_LayoutForRightControls = new QVBoxLayout();
  m_LayoutForRightControls->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForRightControls"));
  m_LayoutForRightControls->setContentsMargins(2, 0, 2, 0);
  m_LayoutForRightControls->setSpacing(0);

  m_LayoutForLayoutButtons = new QHBoxLayout();
  m_LayoutForLayoutButtons->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForLayoutButtons"));
  m_LayoutForLayoutButtons->setContentsMargins(2, 0, 2, 0);
  m_LayoutForLayoutButtons->setSpacing(4);

  m_LayoutForRowsAndColumns = new QHBoxLayout();
  m_LayoutForRowsAndColumns->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForRowsAndColumns"));
  m_LayoutForRowsAndColumns->setContentsMargins(2, 0, 2, 0);
  m_LayoutForRowsAndColumns->setSpacing(4);

  m_LayoutForOrientation = new QHBoxLayout();
  m_LayoutForOrientation->setObjectName(QString::fromUtf8("QmitkMIDASMultiViewWidget::m_LayoutForOrientationBindLink"));
  m_LayoutForOrientation->setContentsMargins(2, 0, 2, 0);
  m_LayoutForOrientation->setSpacing(4);

  m_MIDASOrientationWidget = new QmitkMIDASOrientationWidget(this);
  m_MIDASOrientationWidget->m_SliceOrientationLabel->setVisible(false);
  m_MIDASOrientationWidget->m_AxialRadioButton->setLayoutDirection(Qt::RightToLeft);
  m_MIDASOrientationWidget->m_SagittalRadioButton->setLayoutDirection(Qt::RightToLeft);
  m_MIDASOrientationWidget->m_CoronalRadioButton->setLayoutDirection(Qt::RightToLeft);
  m_MIDASOrientationWidget->m_OrthogonalRadioButton->setLayoutDirection(Qt::RightToLeft);
  m_MIDASOrientationWidget->m_ThreeDRadioButton->setLayoutDirection(Qt::RightToLeft);

  m_MIDASSlidersWidget = new QmitkMIDASSlidersWidget(this);

  m_BindWindowsCheckBox = new QCheckBox(this);
  m_BindWindowsCheckBox->setText("bind");
  m_BindWindowsCheckBox->setToolTip("control multiple viewers using slice, magnification, time and orientation.");
  m_BindWindowsCheckBox->setWhatsThis("bind windows together so that the slice selection, magnification, time-step selection and orientation from the selected window is propagated across all viewers. ");
  m_BindWindowsCheckBox->setLayoutDirection(Qt::RightToLeft);

  m_LinkWindowsCheckBox = new QCheckBox(this);
  m_LinkWindowsCheckBox->setText("link");
  m_LinkWindowsCheckBox->setToolTip("control multiple viewers using linked mouse cursors");
  m_LinkWindowsCheckBox->setWhatsThis("link viewers together so that as the mouse cursor moves, each viewer centres on the same 3D coordinate.");
  m_LinkWindowsCheckBox->setLayoutDirection(Qt::RightToLeft);

  m_1x1LayoutButton = new QPushButton(this);
  m_1x1LayoutButton->setText("1x1");
  m_1x1LayoutButton->setToolTip("display 1 row and 1 column of image viewers");

  m_1x2LayoutButton = new QPushButton(this);
  m_1x2LayoutButton->setText("1x2");
  m_1x2LayoutButton->setToolTip("display 1 row and 2 columns of image viewers");

  m_2x1LayoutButton = new QPushButton(this);
  m_2x1LayoutButton->setText("2x1");
  m_2x1LayoutButton->setToolTip("display 2 rows and 1 column of image viewers");

  m_1x3LayoutButton = new QPushButton(this);
  m_1x3LayoutButton->setText("1x3");
  m_1x3LayoutButton->setToolTip("display 1 row and 3 columns of image viewers");

  m_3x1LayoutButton = new QPushButton(this);
  m_3x1LayoutButton->setText("3x1");
  m_3x1LayoutButton->setToolTip("display 3 rows and 1 column of image viewers");

  m_2x2LayoutButton = new QPushButton(this);
  m_2x2LayoutButton->setText("2x2");
  m_2x2LayoutButton->setToolTip("display 2 rows and 2 columns of image viewers");

  m_3x2LayoutButton = new QPushButton(this);
  m_3x2LayoutButton->setText("3x2");
  m_3x2LayoutButton->setToolTip("display 3 rows and 2 columns of image viewers");

  m_2x3LayoutButton = new QPushButton(this);
  m_2x3LayoutButton->setText("2x3");
  m_2x3LayoutButton->setToolTip("display 2 rows and 2 columns of image viewers");

  m_5x5LayoutButton = new QPushButton(this);
  m_5x5LayoutButton->setText("5x5");
  m_5x5LayoutButton->setToolTip("display 5 rows and 5 columns of image viewers");

  m_RowsSpinBox = new QSpinBox(this);
  m_RowsSpinBox->setMinimum(1);
  m_RowsSpinBox->setMaximum(m_MaxRows);
  m_RowsSpinBox->setValue(1);
  m_RowsSpinBox->setToolTip("click the arrows or type to change the number of rows");

  m_RowsLabel = new QLabel(this);
  m_RowsLabel->setText("rows");

  m_ColumnsSpinBox = new QSpinBox(this);
  m_ColumnsSpinBox->setMinimum(1);
  m_ColumnsSpinBox->setMaximum(m_MaxCols);
  m_ColumnsSpinBox->setValue(1);
  m_ColumnsSpinBox->setToolTip("click the arrows or type to change the number of columns");

  m_ColumnsLabel = new QLabel(this);
  m_ColumnsLabel->setText("columns");

  m_DropLabel = new QLabel(this);
  m_DropLabel->setText("drop:");

  m_DropSingleRadioButton = new QRadioButton(this);
  m_DropSingleRadioButton->setText("single");
  m_DropSingleRadioButton->setToolTip("drop images into a single window");

  m_DropMultipleRadioButton = new QRadioButton(this);
  m_DropMultipleRadioButton->setText("multiple");
  m_DropMultipleRadioButton->setToolTip("drop images across multiple windows");

  m_DropThumbnailRadioButton = new QRadioButton(this);
  m_DropThumbnailRadioButton->setText("all");
  m_DropThumbnailRadioButton->setToolTip("drop multiple images into any window, and the application will spread them across all windows and provide evenly spaced slices through the image");

  for (unsigned int i = 0; i < m_MaxRows*m_MaxCols; i++)
  {
    QmitkMIDASSingleViewWidget *widget = NULL;
    widget = this->CreateSingleViewWidget();

    m_VisibilityManager->RegisterWidget(widget);
    m_SingleViewWidgets.push_back(widget);

    m_LayoutForStackingRenderWindows->insertWidget(i, widget);

    if (i > 0)
    {
      widget->SetEnabled(false);
    }
  }

  /************************************
   * Now arrange stuff.
   ************************************/

  m_LayoutForRenderWindows->addLayout(m_LayoutForStackingRenderWindows, 0, 0);

  m_LayoutForLayoutButtons->addWidget(m_1x1LayoutButton);
  m_LayoutForLayoutButtons->addWidget(m_1x2LayoutButton);
  m_LayoutForLayoutButtons->addWidget(m_1x3LayoutButton);
  m_LayoutForLayoutButtons->addWidget(m_2x2LayoutButton);

  m_LayoutForRowsAndColumns->addWidget(m_RowsLabel);
  m_LayoutForRowsAndColumns->addWidget(m_RowsSpinBox);
  m_LayoutForRowsAndColumns->addWidget(m_ColumnsLabel);
  m_LayoutForRowsAndColumns->addWidget(m_ColumnsSpinBox);
  m_LayoutForRowsAndColumns->insertStretch(4, 2);
  m_LayoutForRowsAndColumns->addWidget(m_BindWindowsCheckBox);
  m_LayoutForRowsAndColumns->addWidget(m_LinkWindowsCheckBox);

  m_LayoutForOrientation->addWidget(m_MIDASOrientationWidget);

  m_LayoutForRightControls->addLayout(m_LayoutForLayoutButtons);
  m_LayoutForRightControls->addLayout(m_LayoutForRowsAndColumns);
  m_LayoutForRightControls->addLayout(m_LayoutForOrientation);

  // Probably don't need this label anymore, so making it invisible for now.
  m_LayoutForTopControls->addWidget(m_DropLabel, 0, 0);
  m_DropLabel->setVisible(false);

  m_LayoutForTopControls->addWidget(m_DropSingleRadioButton, 0, 0);
  m_LayoutForTopControls->addWidget(m_DropMultipleRadioButton, 1, 0);
  m_LayoutForTopControls->addWidget(m_DropThumbnailRadioButton, 2, 0);
  m_LayoutForTopControls->setColumnMinimumWidth(1, 20);

  m_LayoutForTopControls->addWidget(m_MIDASSlidersWidget, 0, 2, 3, 1);
  m_LayoutForTopControls->addLayout(m_LayoutForRightControls, 0, 3, 3, 1);

  m_LayoutForTopControls->setColumnStretch(0, 0);
  m_LayoutForTopControls->setColumnStretch(1, 0);
  m_LayoutForTopControls->setColumnStretch(2, 5);
  m_LayoutForTopControls->setColumnStretch(3, 2);

  // Then the layout buttons: and these are invisible, so put them anywhere.
  m_LayoutForTopControls->addWidget(m_2x1LayoutButton, 3, 0);
  m_LayoutForTopControls->addWidget(m_3x1LayoutButton, 3, 1);
  m_LayoutForTopControls->addWidget(m_2x3LayoutButton, 3, 2);
  m_LayoutForTopControls->addWidget(m_3x2LayoutButton, 3, 3);
  m_LayoutForTopControls->addWidget(m_5x5LayoutButton, 3, 4);

  // Trac #1294, these deemed to be 'not so useful', so making UI simpler for now, by making invisible.
  m_2x1LayoutButton->setVisible(false);
  m_3x1LayoutButton->setVisible(false);
  m_2x3LayoutButton->setVisible(false);
  m_3x2LayoutButton->setVisible(false);
  m_5x5LayoutButton->setVisible(false);

  // Now put layouts within layouts.
  m_LayoutToPutControlsOnTopOfWindows->addLayout(m_LayoutForTopControls);
  m_LayoutToPutControlsOnTopOfWindows->addLayout(m_LayoutForRenderWindows);
  m_TopLevelLayout->addLayout(m_LayoutToPutControlsOnTopOfWindows);

  itk::SimpleMemberCommand<QmitkMIDASMultiViewWidget>::Pointer onFocusChangedCommand =
    itk::SimpleMemberCommand<QmitkMIDASMultiViewWidget>::New();
  onFocusChangedCommand->SetCallbackFunction( this, &QmitkMIDASMultiViewWidget::OnFocusChanged );

  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  m_FocusManagerObserverTag = focusManager->AddObserver(mitk::FocusEvent(), onFocusChangedCommand);

  connect(m_1x1LayoutButton, SIGNAL(pressed()), this, SLOT(On1x1ButtonPressed()));
  connect(m_1x2LayoutButton, SIGNAL(pressed()), this, SLOT(On1x2ButtonPressed()));
  connect(m_2x1LayoutButton, SIGNAL(pressed()), this, SLOT(On2x1ButtonPressed()));
  connect(m_1x3LayoutButton, SIGNAL(pressed()), this, SLOT(On1x3ButtonPressed()));
  connect(m_3x1LayoutButton, SIGNAL(pressed()), this, SLOT(On3x1ButtonPressed()));
  connect(m_2x2LayoutButton, SIGNAL(pressed()), this, SLOT(On2x2ButtonPressed()));
  connect(m_3x2LayoutButton, SIGNAL(pressed()), this, SLOT(On3x2ButtonPressed()));
  connect(m_2x3LayoutButton, SIGNAL(pressed()), this, SLOT(On2x3ButtonPressed()));
  connect(m_5x5LayoutButton, SIGNAL(pressed()), this, SLOT(On5x5ButtonPressed()));
  connect(m_RowsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnRowsSliderValueChanged(int)));
  connect(m_ColumnsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnColumnsSliderValueChanged(int)));
  connect(m_DropSingleRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnDropSingleRadioButtonToggled(bool)));
  connect(m_DropMultipleRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnDropMultipleRadioButtonToggled(bool)));
  connect(m_DropThumbnailRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnDropThumbnailRadioButtonToggled(bool)));
  connect(m_BindWindowsCheckBox, SIGNAL(clicked(bool)), this, SLOT(OnBindWindowsCheckboxClicked(bool)));
  connect(m_LinkWindowsCheckBox, SIGNAL(clicked(bool)), this, SLOT(OnLinkWindowsCheckboxClicked(bool)));
  connect(m_MIDASSlidersWidget->m_SliceSelectionWidget, SIGNAL(SliceNumberChanged(int, int)), this, SLOT(OnSliceNumberChanged(int, int)));
  connect(m_MIDASSlidersWidget->m_MagnificationFactorWidget, SIGNAL(MagnificationFactorChanged(int, int)), this, SLOT(OnMagnificationFactorChanged(int, int)));
  connect(m_MIDASSlidersWidget->m_TimeSelectionWidget, SIGNAL(IntegerValueChanged(int, int)), this, SLOT(OnTimeChanged(int, int)));
  connect(m_MIDASOrientationWidget->m_AxialRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnOrientationSelected(bool)));
  connect(m_MIDASOrientationWidget->m_CoronalRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnOrientationSelected(bool)));
  connect(m_MIDASOrientationWidget->m_OrthogonalRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnOrientationSelected(bool)));
  connect(m_MIDASOrientationWidget->m_SagittalRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnOrientationSelected(bool)));
  connect(m_MIDASOrientationWidget->m_ThreeDRadioButton, SIGNAL(toggled(bool)), this, SLOT(OnOrientationSelected(bool)));

  m_DropSingleRadioButton->blockSignals(true);
  m_DropSingleRadioButton->setChecked(true);
  m_DropSingleRadioButton->blockSignals(false);

  this->m_VisibilityManager->SetDropType(MIDAS_DROP_TYPE_SINGLE);
  this->SetLayoutSize(m_DefaultNumberOfRows, m_DefaultNumberOfColumns, false);
  this->EnableWidgets(false);
}

QmitkMIDASMultiViewWidget::~QmitkMIDASMultiViewWidget()
{
  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  focusManager->RemoveObserver(m_FocusManagerObserverTag);
}

QmitkMIDASSingleViewWidget* QmitkMIDASMultiViewWidget::CreateSingleViewWidget()
{
  QmitkMIDASSingleViewWidget *widget = new QmitkMIDASSingleViewWidget(this, tr("QmitkRenderWindow"), -5, 20, m_DataStorage);
  widget->setObjectName(tr("QmitkMIDASSingleViewWidget"));

  connect(widget, SIGNAL(NodesDropped(QmitkRenderWindow*, std::vector<mitk::DataNode*>)), m_VisibilityManager, SLOT(OnNodesDropped(QmitkRenderWindow*,std::vector<mitk::DataNode*>)));
  connect(widget, SIGNAL(NodesDropped(QmitkRenderWindow*, std::vector<mitk::DataNode*>)), this, SLOT(OnNodesDropped(QmitkRenderWindow*,std::vector<mitk::DataNode*>)));
  connect(widget, SIGNAL(PositionChanged(QmitkMIDASSingleViewWidget*, mitk::Point3D, mitk::Point3D)), this, SLOT(OnPositionChanged(QmitkMIDASSingleViewWidget*,mitk::Point3D,mitk::Point3D)));

  return widget;
}

void QmitkMIDASMultiViewWidget::RequestUpdateAll()
{
  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    m_SingleViewWidgets[i]->RequestUpdate();
  }
}

void QmitkMIDASMultiViewWidget::SetDefaultInterpolationType(MIDASDefaultInterpolationType interpolationType)
{
  m_VisibilityManager->SetDefaultInterpolationType(interpolationType);
}

void QmitkMIDASMultiViewWidget::SetDefaultViewType(MIDASView view)
{
  m_VisibilityManager->SetDefaultViewType(view);
}

void QmitkMIDASMultiViewWidget::SetDropTypeWidget(MIDASDropType dropType)
{
  if (dropType == MIDAS_DROP_TYPE_SINGLE)
  {
    m_DropSingleRadioButton->setChecked(true);
  }
  else if (dropType == MIDAS_DROP_TYPE_MULTIPLE)
  {
    m_DropMultipleRadioButton->setChecked(true);
  }
  else if (dropType == MIDAS_DROP_TYPE_ALL)
  {
    m_DropThumbnailRadioButton->setChecked(true);
  }
}

void QmitkMIDASMultiViewWidget::SetShowDropTypeWidgets(bool visible)
{
  m_DropSingleRadioButton->setVisible(visible);
  m_DropMultipleRadioButton->setVisible(visible);
  m_DropThumbnailRadioButton->setVisible(visible);
}

void QmitkMIDASMultiViewWidget::SetShowLayoutButtons(bool visible)
{
  m_1x1LayoutButton->setVisible(visible);
  m_1x2LayoutButton->setVisible(visible);
  m_1x3LayoutButton->setVisible(visible);
  m_2x2LayoutButton->setVisible(visible);
}

void QmitkMIDASMultiViewWidget::SetShowMagnificationSlider(bool visible)
{
  m_MIDASSlidersWidget->m_MagnificationFactorWidget->setVisible(visible);
}

void QmitkMIDASMultiViewWidget::SetShow2DCursors(bool visible)
{
  m_Show2DCursors = visible;
  this->Update2DCursorVisibility();
}

bool QmitkMIDASMultiViewWidget::GetShow2DCursors() const
{
  return m_Show2DCursors;
}

void QmitkMIDASMultiViewWidget::SetShow3DViewInOrthoView(bool visible)
{
  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    m_SingleViewWidgets[i]->SetDisplay3DViewInOrthoView(visible);
  }
}

void QmitkMIDASMultiViewWidget::EnableDropTypeWidgets(bool enabled)
{
  m_DropLabel->setEnabled(enabled);
  m_DropSingleRadioButton->setEnabled(enabled);
  m_DropMultipleRadioButton->setEnabled(enabled);
  m_DropThumbnailRadioButton->setEnabled(enabled);
}

void QmitkMIDASMultiViewWidget::EnableSliderWidgets(bool enabled)
{
  m_MIDASSlidersWidget->SetEnabled(enabled);
}

void QmitkMIDASMultiViewWidget::EnableLayoutWidgets(bool enabled)
{
  m_1x1LayoutButton->setEnabled(enabled);
  m_1x2LayoutButton->setEnabled(enabled);
  m_2x1LayoutButton->setEnabled(enabled);
  m_3x1LayoutButton->setEnabled(enabled);
  m_2x2LayoutButton->setEnabled(enabled);
  m_1x3LayoutButton->setEnabled(enabled);
  m_3x2LayoutButton->setEnabled(enabled);
  m_2x3LayoutButton->setEnabled(enabled);
  m_5x5LayoutButton->setEnabled(enabled);
  m_RowsSpinBox->setEnabled(enabled);
  m_RowsLabel->setEnabled(enabled);
  m_ColumnsSpinBox->setEnabled(enabled);
  m_ColumnsLabel->setEnabled(enabled);
}

void QmitkMIDASMultiViewWidget::EnableOrientationWidgets(bool enabled)
{
  m_MIDASOrientationWidget->SetEnabled(enabled);
}

void QmitkMIDASMultiViewWidget::EnableBindWidgets(bool enabled)
{
  m_BindWindowsCheckBox->setEnabled(enabled);
  m_LinkWindowsCheckBox->setEnabled(enabled);
}

void QmitkMIDASMultiViewWidget::EnableWidgets(bool enabled)
{
  this->EnableDropTypeWidgets(enabled);
  this->EnableSliderWidgets(enabled);
  this->EnableLayoutWidgets(enabled);
  this->EnableOrientationWidgets(enabled);
  this->EnableBindWidgets(enabled);
}

void QmitkMIDASMultiViewWidget::SetThumbnailMode(bool enabled)
{
  this->m_IsThumbnailMode = enabled;

  if (enabled)
  {
    m_NumberOfRowsInNonThumbnailMode = m_RowsSpinBox->value();
    m_NumberOfColumnsInNonThumbnailMode = m_ColumnsSpinBox->value();
    this->EnableSliderWidgets(false);
    this->EnableLayoutWidgets(false);
    this->EnableOrientationWidgets(false);
    this->EnableBindWidgets(false);
    this->SetLayoutSize(m_MaxRows, m_MaxCols, true);
  }
  else
  {
    this->EnableSliderWidgets(true);
    this->EnableLayoutWidgets(true);
    this->EnableOrientationWidgets(true);
    this->EnableBindWidgets(true);
    this->SetLayoutSize(m_MaxRows, m_MaxCols, false);
  }
}

bool QmitkMIDASMultiViewWidget::GetThumbnailMode() const
{
  return this->m_IsThumbnailMode;
}

void QmitkMIDASMultiViewWidget::SetMIDASSegmentationMode(bool enabled)
{
  this->m_IsMIDASSegmentationMode = enabled;

  if (enabled)
  {
    this->m_NumberOfRowsBeforeSegmentationMode = m_RowsSpinBox->value();
    this->m_NumberOfColumnsBeforeSegmentationMode = m_ColumnsSpinBox->value();
    this->EnableLayoutWidgets(false);
    this->EnableBindWidgets(false);
    this->SetLayoutSize(1, 1, false);
    this->SetSelectedWindow(0);
    this->UpdateFocusManagerToSelectedViewer();
  }
  else
  {
    this->EnableLayoutWidgets(false);
    this->EnableBindWidgets(false);
    this->SetLayoutSize(m_NumberOfRowsBeforeSegmentationMode, m_NumberOfColumnsBeforeSegmentationMode, false);
  }
}

bool QmitkMIDASMultiViewWidget::GetMIDASSegmentationMode() const
{
  return this->m_IsMIDASSegmentationMode;
}

MIDASView QmitkMIDASMultiViewWidget::GetDefaultOrientationForSegmentation()
{
  assert(m_VisibilityManager);

  MIDASView viewForSegmentation = m_VisibilityManager->GetDefaultViewType();
  if (viewForSegmentation != MIDAS_VIEW_AXIAL && viewForSegmentation != MIDAS_VIEW_SAGITTAL && viewForSegmentation != MIDAS_VIEW_CORONAL)
  {
    viewForSegmentation = MIDAS_VIEW_CORONAL;
  }
  return viewForSegmentation;
}

void QmitkMIDASMultiViewWidget::SetBackgroundColour(mitk::Color colour)
{
  QColor background(colour[0] * 255, colour[1] * 255, colour[2] * 255);

  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    m_SingleViewWidgets[i]->SetBackgroundColor(background);
  }
  this->RequestUpdateAll();
}

void QmitkMIDASMultiViewWidget::SetLayoutSize(unsigned int numberOfRows, unsigned int numberOfColumns, bool isThumbnailMode)
{

  // We need to remember the "previous" number of rows and columns, so when we switch out
  // of thumbnail mode, we know how many rows and columns to revert to.
  if (isThumbnailMode)
  {
    m_NumberOfRowsInNonThumbnailMode = m_RowsSpinBox->value();
    m_NumberOfColumnsInNonThumbnailMode = m_ColumnsSpinBox->value();
  }
  else
  {
    // otherwise we remember the "next" (the number we are being asked for in this method call) number of rows and columns.
    m_NumberOfRowsInNonThumbnailMode = numberOfRows;
    m_NumberOfColumnsInNonThumbnailMode = numberOfColumns;
  }

  // Remember, all widgets are created, we just make them visible/invisible.
  for(unsigned int r = 0; r < m_MaxRows; r++)
  {
    for (unsigned int c = 0; c < m_MaxCols; c++)
    {
      int viewerIndex = this->GetIndexFromRowAndColumn(r, c);

      if (viewerIndex == 0)
      {
        // Viewer 0 always stays where it is, and should always be enabled.
        continue;
      }
      else if (r >= numberOfRows || c >= numberOfColumns)
      {
        int currentIndex = m_LayoutForRenderWindows->indexOf(m_SingleViewWidgets[viewerIndex]);

        if (currentIndex != -1)
        {
          m_LayoutForRenderWindows->removeWidget(m_SingleViewWidgets[viewerIndex]);
          m_LayoutForStackingRenderWindows->addWidget(m_SingleViewWidgets[viewerIndex]);
          m_SingleViewWidgets[viewerIndex]->SetEnabled(false);
          m_SingleViewWidgets[viewerIndex]->setVisible(false);
        }
      }
      else
      {
        int currentIndex = m_LayoutForRenderWindows->indexOf(m_SingleViewWidgets[viewerIndex]);

        if (currentIndex == -1)
        {
          m_LayoutForStackingRenderWindows->removeWidget(m_SingleViewWidgets[viewerIndex]);
          m_LayoutForRenderWindows->addWidget(m_SingleViewWidgets[viewerIndex], r, c);
          m_SingleViewWidgets[viewerIndex]->SetEnabled(true);
          m_SingleViewWidgets[viewerIndex]->setVisible(true);
        }
      }
    } // end for c
  } // end for r

  // viewer 0 is always present and never changes.
  m_LayoutForStackingRenderWindows->setCurrentIndex(0);
  m_LayoutForStackingRenderWindows->activate();

  // Update row/column widget without triggering another layout size change.
  m_RowsSpinBox->blockSignals(true);
  m_RowsSpinBox->setValue(numberOfRows);
  m_RowsSpinBox->blockSignals(false);
  m_ColumnsSpinBox->blockSignals(true);
  m_ColumnsSpinBox->setValue(numberOfColumns);
  m_ColumnsSpinBox->blockSignals(false);

  // Test the current m_Selected window, and reset to 0 if it now points to an invisible window.
  int selectedWindow = m_SelectedWindow;
  if (this->GetRowFromIndex(selectedWindow) >= numberOfRows || this->GetColumnFromIndex(selectedWindow) >= numberOfColumns)
  {
    selectedWindow = 0;
  }
  this->SetSelectedWindow(selectedWindow);

  // Need to make sure the cursors correspond to the currently selected window.
  this->Update2DCursorVisibility();

  // Finally, if we are actually in bound mode, we need to make sure that any new windows that were invisible,
  // but now are visible as a result of changing size are reflecting the geometry of the "currently selected" window.
  if (this->m_BindWindowsCheckBox->isChecked())
  {
    this->UpdateBoundGeometry();
  }
}

void QmitkMIDASMultiViewWidget::SetSelectedWindow(unsigned int selectedIndex)
{
  if (selectedIndex >= 0 && selectedIndex < m_SingleViewWidgets.size())
  {
    m_SelectedWindow = selectedIndex;

    for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
    {
      if (i == selectedIndex)
      {
        m_SingleViewWidgets[i]->SetSelected(true);
        if (!m_LinkWindowsCheckBox->isChecked())
        {
          m_SingleViewWidgets[i]->SetNavigationControllerEventListening(true);
        }
      }
      else
      {
        m_SingleViewWidgets[i]->SetSelected(false);
        if (!m_LinkWindowsCheckBox->isChecked())
        {
          m_SingleViewWidgets[i]->SetNavigationControllerEventListening(false);
        }
      }
    }
    this->Update2DCursorVisibility();
    this->RequestUpdateAll();
  }
}

unsigned int QmitkMIDASMultiViewWidget::GetRowFromIndex(unsigned int i)
{
  if (i < 0 || i >= m_MaxRows*m_MaxCols)
  {
    return 0;
  }
  else
  {
    return i / m_MaxCols; // Note, intentionally integer division
  }
}

unsigned int QmitkMIDASMultiViewWidget::GetColumnFromIndex(unsigned int i)
{
  if (i < 0 || i >= m_MaxRows*m_MaxCols)
  {
    return 0;
  }
  else
  {
    return i % m_MaxCols; // Note, intentionally modulus.
  }
}

unsigned int QmitkMIDASMultiViewWidget::GetIndexFromRowAndColumn(unsigned int r, unsigned int c)
{
  return r*m_MaxCols + c;
}

void QmitkMIDASMultiViewWidget::On1x1ButtonPressed()
{
  this->SetLayoutSize(1,1, false);
}

void QmitkMIDASMultiViewWidget::On1x2ButtonPressed()
{
  this->SetLayoutSize(1,2, false);
}

void QmitkMIDASMultiViewWidget::On2x1ButtonPressed()
{
  this->SetLayoutSize(2,1, false);
}

void QmitkMIDASMultiViewWidget::On3x1ButtonPressed()
{
  this->SetLayoutSize(3,1, false);
}

void QmitkMIDASMultiViewWidget::On1x3ButtonPressed()
{
  this->SetLayoutSize(1,3, false);
}

void QmitkMIDASMultiViewWidget::On2x2ButtonPressed()
{
  this->SetLayoutSize(2,2, false);
}

void QmitkMIDASMultiViewWidget::On3x2ButtonPressed()
{
  this->SetLayoutSize(3,2, false);
}

void QmitkMIDASMultiViewWidget::On2x3ButtonPressed()
{
  this->SetLayoutSize(2,3, false);
}

void QmitkMIDASMultiViewWidget::On5x5ButtonPressed()
{
  this->SetLayoutSize(5,5, false);
}

void QmitkMIDASMultiViewWidget::OnRowsSliderValueChanged(int r)
{
  this->SetLayoutSize((unsigned int)r, (unsigned int)m_ColumnsSpinBox->value(), false);
}

void QmitkMIDASMultiViewWidget::OnColumnsSliderValueChanged(int c)
{
  this->SetLayoutSize((unsigned int)m_RowsSpinBox->value(), (unsigned int)c, false);
}

void QmitkMIDASMultiViewWidget::OnPositionChanged(QmitkMIDASSingleViewWidget *widget, mitk::Point3D voxelLocation, mitk::Point3D millimetreLocation)
{
  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    if (m_SingleViewWidgets[i] == widget)
    {
      std::vector<QmitkRenderWindow*> windows = m_SingleViewWidgets[i]->GetSelectedWindows();
      if (windows.size() == 1)
      {
        mitk::SliceNavigationController::Pointer snc = windows[0]->GetSliceNavigationController();
        int slice = snc->GetSlice()->GetPos();
        m_MIDASSlidersWidget->m_SliceSelectionWidget->SetSliceNumber(slice);
      }
    }
  }
}

void QmitkMIDASMultiViewWidget::OnNodesDropped(QmitkRenderWindow *window, std::vector<mitk::DataNode*> nodes)
{
  // Make sure that the window that was dropped into is enabled.
  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    if (m_SingleViewWidgets[i]->ContainsWindow(window) && !m_SingleViewWidgets[i]->IsEnabled())
    {
      m_SingleViewWidgets[i]->SetEnabled(true);
    }
  }
  if (!this->m_DropThumbnailRadioButton->isChecked())
  {
    this->EnableWidgets(true);
  }
  mitk::GlobalInteraction::GetInstance()->GetFocusManager()->SetFocused(window->GetRenderer());
}

void QmitkMIDASMultiViewWidget::OnFocusChanged()
{
  vtkRenderWindow* focusedRenderWindow = NULL;
  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  mitk::BaseRenderer::ConstPointer baseRenderer = focusManager->GetFocused();

  int selectedWindow = -1;

  if (baseRenderer.IsNotNull())
  {
    focusedRenderWindow = baseRenderer->GetRenderWindow();
    for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
    {
      if (m_SingleViewWidgets[i]->ContainsVtkRenderWindow(focusedRenderWindow))
      {
        selectedWindow = i;
        break;
      }
    }
  }

  if (selectedWindow != -1)
  {
    // This, to turn off borders on all other windows.
    this->SetSelectedWindow(selectedWindow);

    // This to specifically set the border round one sub-pane for if its an ortho-view.
    this->m_SingleViewWidgets[selectedWindow]->SetSelectedWindow(focusedRenderWindow);

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Need to enable widgets appropriately, so user can't press stuff that they aren't meant to.
    /////////////////////////////////////////////////////////////////////////////////////////////
    MIDASOrientation orientation = this->m_SingleViewWidgets[selectedWindow]->GetOrientation();
    MIDASView view = this->m_SingleViewWidgets[selectedWindow]->GetView();

    m_MIDASSlidersWidget->SetBlockSignals(true);
    m_MIDASOrientationWidget->SetBlockSignals(true);

    if (view != MIDAS_VIEW_UNKNOWN)
    {
      if (view == MIDAS_VIEW_AXIAL)
      {
        m_MIDASOrientationWidget->m_AxialRadioButton->setChecked(true);
      }
      else if (view == MIDAS_VIEW_SAGITTAL)
      {
        m_MIDASOrientationWidget->m_SagittalRadioButton->setChecked(true);
      }
      else if (view == MIDAS_VIEW_CORONAL)
      {
        m_MIDASOrientationWidget->m_CoronalRadioButton->setChecked(true);
      }
      else if (view == MIDAS_VIEW_ORTHO)
      {
        m_MIDASOrientationWidget->m_OrthogonalRadioButton->setChecked(true);
      }
      else if (view == MIDAS_VIEW_3D)
      {
        m_MIDASOrientationWidget->m_ThreeDRadioButton->setChecked(true);
      }
    }
    if (orientation != MIDAS_ORIENTATION_UNKNOWN)
    {
      unsigned int minSlice = this->m_SingleViewWidgets[selectedWindow]->GetMinSlice(orientation);
      unsigned int maxSlice = this->m_SingleViewWidgets[selectedWindow]->GetMaxSlice(orientation);
      unsigned int currentSlice = this->m_SingleViewWidgets[selectedWindow]->GetSliceNumber(orientation);

      m_MIDASSlidersWidget->m_SliceSelectionWidget->SetMinimum(minSlice);
      m_MIDASSlidersWidget->m_SliceSelectionWidget->SetMaximum(maxSlice);
      m_MIDASSlidersWidget->m_SliceSelectionWidget->SetSliceNumber(currentSlice);
      m_MIDASSlidersWidget->m_SliceSelectionWidget->setEnabled(true);

      unsigned int minMag = this->m_SingleViewWidgets[selectedWindow]->GetMinMagnification();
      unsigned int maxMag = this->m_SingleViewWidgets[selectedWindow]->GetMaxMagnification();
      unsigned int currentMag = this->m_SingleViewWidgets[selectedWindow]->GetMagnificationFactor(orientation);
      m_MIDASSlidersWidget->m_MagnificationFactorWidget->SetMinimum(minMag);
      m_MIDASSlidersWidget->m_MagnificationFactorWidget->SetMaximum(maxMag);
      m_MIDASSlidersWidget->m_MagnificationFactorWidget->SetMagnificationFactor(currentMag);
      m_MIDASSlidersWidget->m_MagnificationFactorWidget->setEnabled(true);
    }
    else
    {
      m_MIDASSlidersWidget->m_SliceSelectionWidget->setEnabled(false);
      m_MIDASSlidersWidget->m_MagnificationFactorWidget->setEnabled(false);
    }
    unsigned int minTime = this->m_SingleViewWidgets[selectedWindow]->GetMinTime();
    unsigned int maxTime = this->m_SingleViewWidgets[selectedWindow]->GetMaxTime();
    unsigned int currentTime = this->m_SingleViewWidgets[selectedWindow]->GetTime();
    m_MIDASSlidersWidget->m_TimeSelectionWidget->SetMinimum(minTime);
    m_MIDASSlidersWidget->m_TimeSelectionWidget->SetMaximum(maxTime);
    m_MIDASSlidersWidget->m_TimeSelectionWidget->SetValue(currentTime);

    m_MIDASSlidersWidget->SetBlockSignals(false);
    m_MIDASOrientationWidget->SetBlockSignals(false);
  }
  this->Update2DCursorVisibility();
}

void QmitkMIDASMultiViewWidget::OnDropSingleRadioButtonToggled(bool toggled)
{
  if (toggled)
  {
    m_VisibilityManager->ClearAllWindows();
    m_VisibilityManager->SetDropType(MIDAS_DROP_TYPE_SINGLE);
    this->SetThumbnailMode(false);
  }
}

void QmitkMIDASMultiViewWidget::OnDropMultipleRadioButtonToggled(bool toggled)
{
  if (toggled)
  {
    m_VisibilityManager->ClearAllWindows();
    m_VisibilityManager->SetDropType(MIDAS_DROP_TYPE_MULTIPLE);
    this->SetThumbnailMode(false);
  }
}

void QmitkMIDASMultiViewWidget::OnDropThumbnailRadioButtonToggled(bool toggled)
{
  if (toggled)
  {
    m_VisibilityManager->ClearAllWindows();
    m_VisibilityManager->SetDropType(MIDAS_DROP_TYPE_ALL);
    this->SetThumbnailMode(true);
  }
}

std::vector<unsigned int> QmitkMIDASMultiViewWidget::GetViewerIndexesToUpdate(bool doAllVisible, bool isTimeStep)
{
  std::vector<unsigned int> result;

  if (this->m_BindWindowsCheckBox->isChecked() || (isTimeStep && this->m_DropThumbnailRadioButton->isChecked()) || doAllVisible)
  {
    // In bind mode, or time stepping in thumbnail mode, we want all the viewers that are currently on screen.
    for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
    {
      if (m_SingleViewWidgets[i]->isVisible())
      {
        result.push_back(i);
      }
    }
  }
  else
  {
    // In unbound mode, we just put the currently selected window.
    if (m_SelectedWindow >= 0)
    {
      result.push_back((unsigned int)m_SelectedWindow);
    }
  }
  return result;
}

bool QmitkMIDASMultiViewWidget::MoveAnterior()
{
  bool actuallyDidSomething = false;
  MIDASOrientation orientation = this->m_SingleViewWidgets[m_SelectedWindow]->GetOrientation();
  unsigned int currentSlice = this->m_SingleViewWidgets[m_SelectedWindow]->GetSliceNumber(orientation);
  unsigned int maxSlice = this->m_SingleViewWidgets[m_SelectedWindow]->GetMaxSlice(orientation);
  unsigned int nextSlice = currentSlice+1;
  if (nextSlice <= maxSlice)
  {
    this->SetSelectedWindowSliceNumber(nextSlice);
    actuallyDidSomething = true;
  }
  return actuallyDidSomething;
}

bool QmitkMIDASMultiViewWidget::MovePosterior()
{
  bool actuallyDidSomething = false;
  MIDASOrientation orientation = this->m_SingleViewWidgets[m_SelectedWindow]->GetOrientation();
  unsigned int currentSlice = this->m_SingleViewWidgets[m_SelectedWindow]->GetSliceNumber(orientation);
  unsigned int minSlice = this->m_SingleViewWidgets[m_SelectedWindow]->GetMinSlice(orientation);
  if (currentSlice > 0)
  {
    unsigned int nextSlice = currentSlice-1;
    if (nextSlice >= minSlice)
    {
      this->SetSelectedWindowSliceNumber(nextSlice);
      actuallyDidSomething = true;
    }
  }
  return actuallyDidSomething;
}

void QmitkMIDASMultiViewWidget::OnSliceNumberChanged(int previousSlice, int currentSlice)
{
  this->SetSelectedWindowSliceNumber(currentSlice);
}

void QmitkMIDASMultiViewWidget::SetSelectedWindowSliceNumber(int sliceNumber)
{
  MIDASOrientation orientation = this->m_SingleViewWidgets[m_SelectedWindow]->GetOrientation();
  if (orientation != MIDAS_ORIENTATION_UNKNOWN)
  {
    std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(false, false);

    for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
    {
      this->m_SingleViewWidgets[viewersToUpdate[i]]->SetSliceNumber(orientation, sliceNumber);
    }
  }
}

void QmitkMIDASMultiViewWidget::OnMagnificationFactorChanged(int previousMagnification, int currentMagnification)
{
  this->SetSelectedWindowMagnification(currentMagnification);
}

void QmitkMIDASMultiViewWidget::SetSelectedWindowMagnification(int magnificationFactor)
{
  MIDASOrientation orientation = this->m_SingleViewWidgets[m_SelectedWindow]->GetOrientation();
  if (orientation != MIDAS_ORIENTATION_UNKNOWN)
  {
    std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(false, false);

    for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
    {
      this->m_SingleViewWidgets[viewersToUpdate[i]]->SetMagnificationFactor(orientation, magnificationFactor);
    }
  }
}

void QmitkMIDASMultiViewWidget::OnTimeChanged(int previousTime, int currentTime)
{
  this->SetSelectedTimeStep(currentTime);
}

void QmitkMIDASMultiViewWidget::SetSelectedTimeStep(int timeStep)
{
  std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(false, true);
  for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
  {
    this->m_SingleViewWidgets[viewersToUpdate[i]]->SetTime(timeStep);
  }
}

void QmitkMIDASMultiViewWidget::OnOrientationSelected(bool toggled)
{
  if (toggled)
  {
    if (m_MIDASOrientationWidget->m_AxialRadioButton->isChecked())
    {
      this->SwitchView(MIDAS_VIEW_AXIAL);
    }
    else if (m_MIDASOrientationWidget->m_CoronalRadioButton->isChecked())
    {
      this->SwitchView(MIDAS_VIEW_CORONAL);
    }
    else if (m_MIDASOrientationWidget->m_SagittalRadioButton->isChecked())
    {
      this->SwitchView(MIDAS_VIEW_SAGITTAL);
    }
    else if (m_MIDASOrientationWidget->m_OrthogonalRadioButton->isChecked())
    {
      this->SwitchView(MIDAS_VIEW_ORTHO);
    }
    else if (m_MIDASOrientationWidget->m_ThreeDRadioButton->isChecked())
    {
      this->SwitchView(MIDAS_VIEW_3D);
    }
    // Update the focus to the selected window, to trigger things like thumbnail viewer refresh
    // (or indeed anything that's listening to the FocusManager).
    this->UpdateFocusManagerToSelectedViewer();

  }
}

void QmitkMIDASMultiViewWidget::UpdateFocusManagerToSelectedViewer()
{
  int selectedWindow = this->m_SelectedWindow;
  std::vector<QmitkRenderWindow*> windows = this->m_SingleViewWidgets[selectedWindow]->GetSelectedWindows();

  if (windows.size() > 0)
  {
    mitk::GlobalInteraction::GetInstance()->GetFocusManager()->SetFocused(windows[0]->GetRenderer());
  }
}

bool QmitkMIDASMultiViewWidget::SwitchToAxial()
{
  this->SetSelectedWindowToAxial();
  m_MIDASOrientationWidget->blockSignals(true);
  m_MIDASOrientationWidget->m_AxialRadioButton->blockSignals(true);
  m_MIDASOrientationWidget->m_AxialRadioButton->setChecked(true);
  m_MIDASOrientationWidget->m_AxialRadioButton->blockSignals(false);
  m_MIDASOrientationWidget->blockSignals(false);
  this->UpdateFocusManagerToSelectedViewer();
  return true;
}

void QmitkMIDASMultiViewWidget::SetSelectedWindowToAxial()
{
  this->SwitchView(MIDAS_VIEW_AXIAL);
}

bool QmitkMIDASMultiViewWidget::SwitchToSagittal()
{
  this->SetSelectedWindowToSagittal();
  m_MIDASOrientationWidget->blockSignals(true);
  m_MIDASOrientationWidget->m_SagittalRadioButton->blockSignals(true);
  m_MIDASOrientationWidget->m_SagittalRadioButton->setChecked(true);
  m_MIDASOrientationWidget->m_SagittalRadioButton->blockSignals(false);
  m_MIDASOrientationWidget->blockSignals(false);
  this->UpdateFocusManagerToSelectedViewer();
  return true;
}

void QmitkMIDASMultiViewWidget::SetSelectedWindowToSagittal()
{
  this->SwitchView(MIDAS_VIEW_SAGITTAL);
}

bool QmitkMIDASMultiViewWidget::SwitchToCoronal()
{
  this->SetSelectedWindowToCoronal();
  m_MIDASOrientationWidget->blockSignals(true);
  m_MIDASOrientationWidget->m_CoronalRadioButton->blockSignals(true);
  m_MIDASOrientationWidget->m_CoronalRadioButton->setChecked(true);
  m_MIDASOrientationWidget->m_CoronalRadioButton->blockSignals(false);
  m_MIDASOrientationWidget->blockSignals(false);
  this->UpdateFocusManagerToSelectedViewer();
  return true;
}

void QmitkMIDASMultiViewWidget::SetSelectedWindowToCoronal()
{
  this->SwitchView(MIDAS_VIEW_CORONAL);
}

void QmitkMIDASMultiViewWidget::SwitchView(MIDASView view)
{
  std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(false, true);
  for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
  {
    unsigned int viewerNumber = viewersToUpdate[i];

    this->m_SingleViewWidgets[viewerNumber]->SetView(view, false);

    if ((int)viewerNumber == this->m_SelectedWindow)
    {
      if (view == MIDAS_VIEW_AXIAL)
      {
        this->m_SingleViewWidgets[viewerNumber]
              ->SetSelectedWindow(this->m_SingleViewWidgets[viewerNumber]->GetAxialWindow()->GetVtkRenderWindow());
      }
      else if (view == MIDAS_VIEW_SAGITTAL)
      {
        this->m_SingleViewWidgets[viewerNumber]
              ->SetSelectedWindow(this->m_SingleViewWidgets[viewerNumber]->GetSagittalWindow()->GetVtkRenderWindow());
      }
      else if (view == MIDAS_VIEW_CORONAL)
      {
        this->m_SingleViewWidgets[viewerNumber]
              ->SetSelectedWindow(this->m_SingleViewWidgets[viewerNumber]->GetCoronalWindow()->GetVtkRenderWindow());
      }
    }
  }
}

void QmitkMIDASMultiViewWidget::UpdateBoundGeometry()
{
  mitk::TimeSlicedGeometry::Pointer selectedGeometry = m_SingleViewWidgets[m_SelectedWindow]->GetGeometry();
  MIDASOrientation orientation = m_SingleViewWidgets[m_SelectedWindow]->GetOrientation();
  MIDASView view = m_SingleViewWidgets[m_SelectedWindow]->GetView();
  int sliceNumber = m_SingleViewWidgets[m_SelectedWindow]->GetSliceNumber(orientation);
  int magnification = m_SingleViewWidgets[m_SelectedWindow]->GetMagnificationFactor(orientation);
  int timeStepNumber = m_SingleViewWidgets[m_SelectedWindow]->GetTime();

  std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(false, false);
  for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
  {
    unsigned int viewerIndex = viewersToUpdate[i];

    m_SingleViewWidgets[viewerIndex]->SetBoundGeometry(selectedGeometry);
    m_SingleViewWidgets[viewerIndex]->SetView(view, false);
    m_SingleViewWidgets[viewerIndex]->SetSliceNumber(orientation, sliceNumber);
    m_SingleViewWidgets[viewerIndex]->SetMagnificationFactor(orientation, magnification);
    m_SingleViewWidgets[viewerIndex]->SetTime(timeStepNumber);
  }
}

void QmitkMIDASMultiViewWidget::OnBindWindowsCheckboxClicked(bool isBound)
{
  for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
  {
    m_SingleViewWidgets[i]->SetBound(isBound);
  }

  if (isBound)
  {
    m_LinkWindowsCheckBox->blockSignals(true);
    m_LinkWindowsCheckBox->setChecked(false);
    m_LinkWindowsCheckBox->blockSignals(false);

    this->UpdateBoundGeometry();
    this->Update2DCursorVisibility();
  }
}

void QmitkMIDASMultiViewWidget::Update2DCursorVisibility()
{
  std::vector<unsigned int> viewersToUpdate = this->GetViewerIndexesToUpdate(true, false);

  for (unsigned int i = 0; i < viewersToUpdate.size(); i++)
  {
    bool globalVisibility = false;
    bool localVisibility = m_Show2DCursors;
    m_SingleViewWidgets[viewersToUpdate[i]]->SetDisplay2DCursorsGlobally(globalVisibility);
    m_SingleViewWidgets[viewersToUpdate[i]]->SetDisplay2DCursorsLocally(localVisibility);
  }

  this->RequestUpdateAll();
}

void QmitkMIDASMultiViewWidget::OnLinkWindowsCheckboxClicked(bool isLinked)
{
  if (isLinked)
  {
    m_BindWindowsCheckBox->blockSignals(true);
    m_BindWindowsCheckBox->setChecked(false);
    m_BindWindowsCheckBox->blockSignals(false);

    for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
    {
      m_SingleViewWidgets[i]->SetNavigationControllerEventListening(true);
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_SingleViewWidgets.size(); i++)
    {
      if (i == m_SelectedWindow)
      {
        m_SingleViewWidgets[i]->SetNavigationControllerEventListening(true);
      }
      else
      {
        m_SingleViewWidgets[i]->SetNavigationControllerEventListening(false);
      }
    }
  }
  this->Update2DCursorVisibility();
}
