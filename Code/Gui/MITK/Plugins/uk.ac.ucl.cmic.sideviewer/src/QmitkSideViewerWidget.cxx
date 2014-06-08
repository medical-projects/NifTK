/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "QmitkSideViewerWidget.h"

#include "QmitkBaseView.h"

#include <berryIWorkbenchPage.h>

#include <itkCommand.h>

#include <mitkBaseRenderer.h>
#include <mitkDataStorage.h>
#include <mitkFocusManager.h>
#include <mitkGeometry3D.h>
#include <mitkGlobalInteraction.h>
#include <mitkIRenderWindowPart.h>
#include <mitkSliceNavigationController.h>

#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>


class EditorLifeCycleListener : public berry::IPartListener
{
  berryObjectMacro(EditorLifeCycleListener)

public:

  EditorLifeCycleListener(QmitkSideViewerWidget* sideViewerWidget)
  : m_SideViewerWidget(sideViewerWidget)
  {
  }

private:

  Events::Types GetPartEventTypes() const
  {
    return Events::VISIBLE;
  }

  void PartVisible(berry::IWorkbenchPartReference::Pointer partRef)
  {
    berry::IWorkbenchPart* part = partRef->GetPart(false).GetPointer();

    if (mitk::IRenderWindowPart* renderWindowPart = dynamic_cast<mitk::IRenderWindowPart*>(part))
    {
      mitk::BaseRenderer* focusedRenderer = mitk::GlobalInteraction::GetInstance()->GetFocus();

      bool found = false;
      /// Note:
      /// We need to look for the focused window among every window of the editor.
      /// The MITK Display has not got the concept of 'selected' window and always
      /// returns the axial window as 'active'. Therefore we cannot use GetActiveQmitkRenderWindow.
      foreach (QmitkRenderWindow* mainWindow, renderWindowPart->GetQmitkRenderWindows().values())
      {
        if (focusedRenderer == mainWindow->GetRenderer())
        {
          m_SideViewerWidget->OnMainWindowChanged(renderWindowPart, mainWindow);
          found = true;
          break;
        }
      }

      if (!found)
      {
        QmitkRenderWindow* mainWindow = renderWindowPart->GetActiveQmitkRenderWindow();
        if (mainWindow && mainWindow->isVisible())
        {
          m_SideViewerWidget->OnMainWindowChanged(renderWindowPart, mainWindow);
        }
      }
    }
  }

  QmitkSideViewerWidget* m_SideViewerWidget;

};


//-----------------------------------------------------------------------------
QmitkSideViewerWidget::QmitkSideViewerWidget(QmitkBaseView* view, QWidget* parent)
: m_ContainingView(view)
, m_FocusManagerObserverTag(0)
, m_WindowLayout(WINDOW_LAYOUT_UNKNOWN)
, m_MainWindow(0)
, m_MainAxialWindow(0)
, m_MainSagittalWindow(0)
, m_MainCoronalWindow(0)
, m_MainWindowSnc(0)
, m_MainAxialSnc(0)
, m_MainSagittalSnc(0)
, m_MainCoronalSnc(0)
, m_NodeAddedSetter(0)
, m_VisibilityTracker(0)
, m_Magnification(0.0)
, m_MainWindowOrientation(MIDAS_ORIENTATION_UNKNOWN)
, m_SingleWindowLayouts()
, m_MIDASToolNodeNameFilter(0)
, m_TimeGeometry(0)
{
  this->setupUi(parent);

  m_EditorLifeCycleListener = new EditorLifeCycleListener(this);
  m_ContainingView->GetSite()->GetPage()->AddPartListener(m_EditorLifeCycleListener);

  m_Viewer->SetShow3DWindowIn2x2WindowLayout(false);

  m_CoronalWindowRadioButton->setChecked(true);

  m_SingleWindowLayouts[MIDAS_ORIENTATION_AXIAL] = WINDOW_LAYOUT_CORONAL;
  m_SingleWindowLayouts[MIDAS_ORIENTATION_SAGITTAL] = WINDOW_LAYOUT_CORONAL;
  m_SingleWindowLayouts[MIDAS_ORIENTATION_CORONAL] = WINDOW_LAYOUT_SAGITTAL;

  m_MagnificationSpinBox->setDecimals(2);
  m_MagnificationSpinBox->setSingleStep(1.0);

  double minMagnification = std::ceil(m_Viewer->GetMinMagnification());
  double maxMagnification = std::floor(m_Viewer->GetMaxMagnification());

  m_MagnificationSpinBox->setMinimum(minMagnification);
  m_MagnificationSpinBox->setMaximum(maxMagnification);

  m_ControlsWidget->setEnabled(false);

  std::vector<const mitk::BaseRenderer*> renderers;
  renderers.push_back(m_Viewer->GetAxialWindow()->GetRenderer());
  renderers.push_back(m_Viewer->GetSagittalWindow()->GetRenderer());
  renderers.push_back(m_Viewer->GetCoronalWindow()->GetRenderer());

  m_NodeAddedSetter = mitk::DataNodeAddedVisibilitySetter::New();
  m_MIDASToolNodeNameFilter = mitk::MIDASDataNodeNameStringFilter::New();
  m_NodeAddedSetter->AddFilter(m_MIDASToolNodeNameFilter.GetPointer());
  m_NodeAddedSetter->SetRenderers(renderers);
  m_NodeAddedSetter->SetVisibility(false);

  m_VisibilityTracker = mitk::DataStorageVisibilityTracker::New();
  m_VisibilityTracker->SetNodesToIgnore(m_Viewer->GetWidgetPlanes());
  m_VisibilityTracker->SetManagedRenderers(renderers);

  m_Viewer->SetCursorVisible(true);
  m_Viewer->SetRememberSettingsPerWindowLayout(false);
  m_Viewer->SetDisplayInteractionsEnabled(true);
  m_Viewer->SetCursorPositionBinding(false);
  m_Viewer->SetScaleFactorBinding(true);

  this->connect(m_AxialWindowRadioButton, SIGNAL(toggled(bool)), SLOT(OnAxialWindowRadioButtonToggled(bool)));
  this->connect(m_SagittalWindowRadioButton, SIGNAL(toggled(bool)), SLOT(OnSagittalWindowRadioButtonToggled(bool)));
  this->connect(m_CoronalWindowRadioButton, SIGNAL(toggled(bool)), SLOT(OnCoronalWindowRadioButtonToggled(bool)));
  this->connect(m_MultiWindowRadioButton, SIGNAL(toggled(bool)), SLOT(OnMultiWindowRadioButtonToggled(bool)));
  this->connect(m_MultiWindowComboBox, SIGNAL(currentIndexChanged(int)), SLOT(OnMultiWindowComboBoxIndexChanged()));

  this->connect(m_Viewer, SIGNAL(WindowLayoutChanged(niftkSingleViewerWidget*, WindowLayout)), SLOT(OnWindowLayoutChanged(niftkSingleViewerWidget*, WindowLayout)));

  this->connect(m_SliceSpinBox, SIGNAL(valueChanged(int)), SLOT(OnSliceSpinBoxValueChanged(int)));
  this->connect(m_Viewer, SIGNAL(SelectedPositionChanged(niftkSingleViewerWidget*, const mitk::Point3D&)), SLOT(OnSelectedPositionChanged(niftkSingleViewerWidget*, const mitk::Point3D&)));
  this->connect(m_MagnificationSpinBox, SIGNAL(valueChanged(double)), SLOT(OnMagnificationSpinBoxValueChanged(double)));
  this->connect(m_Viewer, SIGNAL(ScaleFactorChanged(niftkSingleViewerWidget*, MIDASOrientation, double)), SLOT(OnScaleFactorChanged(niftkSingleViewerWidget*, MIDASOrientation, double)));

  // Register focus observer.
  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  if (focusManager)
  {
    itk::SimpleMemberCommand<QmitkSideViewerWidget>::Pointer onFocusChangedCommand =
      itk::SimpleMemberCommand<QmitkSideViewerWidget>::New();
    onFocusChangedCommand->SetCallbackFunction( this, &QmitkSideViewerWidget::OnFocusChanged );

    m_FocusManagerObserverTag = focusManager->AddObserver(mitk::FocusEvent(), onFocusChangedCommand);
  }

  mitk::IRenderWindowPart* selectedEditor = this->GetSelectedEditor();
  if (selectedEditor)
  {
    bool found = false;

    mitk::BaseRenderer* focusedRenderer = focusManager->GetFocused();

    /// Note:
    /// We need to look for the focused window among every window of the editor.
    /// The MITK Display has not got the concept of 'selected' window and always
    /// returns the axial window as 'active'. Therefore we cannot use GetActiveQmitkRenderWindow.
    foreach (QmitkRenderWindow* mainWindow, selectedEditor->GetQmitkRenderWindows().values())
    {
      if (focusedRenderer == mainWindow->GetRenderer())
      {
        this->OnMainWindowChanged(selectedEditor, mainWindow);
        found = true;
        break;
      }
    }

    if (!found)
    {
      QmitkRenderWindow* mainWindow = selectedEditor->GetActiveQmitkRenderWindow();
      if (mainWindow && mainWindow->isVisible())
      {
        this->OnMainWindowChanged(selectedEditor, mainWindow);
      }
    }
  }
}


//-----------------------------------------------------------------------------
QmitkSideViewerWidget::~QmitkSideViewerWidget()
{
  m_ContainingView->GetSite()->GetPage()->RemovePartListener(m_EditorLifeCycleListener);

  m_VisibilityTracker->SetTrackedRenderer(0);
  m_Viewer->SetEnabled(false);

  if (m_MainWindow)
  {
    m_MainWindowSnc->Disconnect(this);
  }
  if (m_MainAxialWindow)
  {
    m_MainAxialSnc->Disconnect(m_Viewer->GetAxialWindow()->GetSliceNavigationController());
    m_Viewer->GetAxialWindow()->GetSliceNavigationController()->Disconnect(m_MainAxialSnc);
  }
  if (m_MainSagittalWindow)
  {
    m_MainSagittalSnc->Disconnect(m_Viewer->GetSagittalWindow()->GetSliceNavigationController());
    m_Viewer->GetSagittalWindow()->GetSliceNavigationController()->Disconnect(m_MainSagittalSnc);
  }
  if (m_MainCoronalWindow)
  {
    m_MainCoronalSnc->Disconnect(m_Viewer->GetCoronalWindow()->GetSliceNavigationController());
    m_Viewer->GetCoronalWindow()->GetSliceNavigationController()->Disconnect(m_MainCoronalSnc);
  }

  // Deregister focus observer.
  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  if (focusManager)
  {
    focusManager->RemoveObserver(m_FocusManagerObserverTag);
  }

  /// m_NodeAddedSetter deleted by smart pointer.
  /// m_VisibilityTracker deleted by smart pointer.
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::SetDataStorage(mitk::DataStorage* dataStorage)
{
  if (dataStorage)
  {
    m_Viewer->SetDataStorage(dataStorage);

    m_NodeAddedSetter->SetDataStorage(dataStorage);
    m_VisibilityTracker->SetDataStorage(dataStorage);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnAMainWindowDestroyed(QObject* mainWindow)
{
  if (mainWindow == m_MainWindow)
  {
    m_VisibilityTracker->SetTrackedRenderer(0);
    m_Viewer->SetEnabled(false);
    m_MainWindow = 0;
    m_MainWindowSnc = 0;
  }

  if (mainWindow == m_MainAxialWindow)
  {
    m_Viewer->GetAxialWindow()->GetSliceNavigationController()->Disconnect(m_MainAxialSnc);
    m_MainAxialWindow = 0;
    m_MainAxialSnc = 0;
  }
  else if (mainWindow == m_MainSagittalWindow)
  {
    m_Viewer->GetSagittalWindow()->GetSliceNavigationController()->Disconnect(m_MainSagittalSnc);
    m_MainSagittalWindow = 0;
    m_MainSagittalSnc = 0;
  }
  else if (mainWindow == m_MainCoronalWindow)
  {
    m_Viewer->GetCoronalWindow()->GetSliceNavigationController()->Disconnect(m_MainCoronalSnc);
    m_MainCoronalWindow = 0;
    m_MainCoronalSnc = 0;
  }
  else
  {
    /// This should not happen.
    assert(false);
  }

  m_Viewer->RequestUpdate();
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnAxialWindowRadioButtonToggled(bool checked)
{
  if (checked)
  {
    m_SingleWindowLayouts[m_MainWindowOrientation] = WINDOW_LAYOUT_AXIAL;
    m_Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnSagittalWindowRadioButtonToggled(bool checked)
{
  if (checked)
  {
    m_SingleWindowLayouts[m_MainWindowOrientation] = WINDOW_LAYOUT_SAGITTAL;
    m_Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnCoronalWindowRadioButtonToggled(bool checked)
{
  if (checked)
  {
    m_SingleWindowLayouts[m_MainWindowOrientation] = WINDOW_LAYOUT_CORONAL;
    m_Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnMultiWindowRadioButtonToggled(bool checked)
{
  if (checked)
  {
    WindowLayout windowLayout = this->GetMultiWindowLayoutForOrientation(m_MainWindowOrientation);
    m_Viewer->SetWindowLayout(windowLayout);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnMultiWindowComboBoxIndexChanged()
{
  if (!m_MultiWindowRadioButton->isChecked())
  {
    bool wasBlocked = m_MultiWindowRadioButton->blockSignals(true);
    m_MultiWindowRadioButton->setChecked(true);
    m_MultiWindowRadioButton->blockSignals(wasBlocked);
  }

  WindowLayout windowLayout = this->GetMultiWindowLayoutForOrientation(m_MainWindowOrientation);
  m_Viewer->SetWindowLayout(windowLayout);
}


//-----------------------------------------------------------------------------
WindowLayout QmitkSideViewerWidget::GetMultiWindowLayoutForOrientation(MIDASOrientation mainWindowOrientation)
{
  WindowLayout windowLayout = WINDOW_LAYOUT_UNKNOWN;

  // 2H
  if (m_MultiWindowComboBox->currentIndex() == 0)
  {
    if (mainWindowOrientation == MIDAS_ORIENTATION_AXIAL)
    {
      windowLayout = WINDOW_LAYOUT_COR_SAG_H;
    }
    else if (mainWindowOrientation == MIDAS_ORIENTATION_SAGITTAL)
    {
      windowLayout = WINDOW_LAYOUT_COR_AX_H;
    }
    else if (mainWindowOrientation == MIDAS_ORIENTATION_CORONAL)
    {
      windowLayout = WINDOW_LAYOUT_SAG_AX_H;
    }
  }
  // 2V
  else if (m_MultiWindowComboBox->currentIndex() == 1)
  {
    if (mainWindowOrientation == MIDAS_ORIENTATION_AXIAL)
    {
      windowLayout = WINDOW_LAYOUT_COR_SAG_V;
    }
    else if (mainWindowOrientation == MIDAS_ORIENTATION_SAGITTAL)
    {
      windowLayout = WINDOW_LAYOUT_COR_AX_V;
    }
    else if (mainWindowOrientation == MIDAS_ORIENTATION_CORONAL)
    {
      windowLayout = WINDOW_LAYOUT_SAG_AX_V;
    }
  }

  return windowLayout;
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnMainWindowOrientationChanged(MIDASOrientation mainWindowOrientation)
{
  WindowLayout windowLayout = WINDOW_LAYOUT_UNKNOWN;

  bool wasBlocked = m_LayoutWidget->blockSignals(true);

  WindowLayout defaultMultiWindowLayout = this->GetMultiWindowLayoutForOrientation(mainWindowOrientation);

  if (!m_MultiWindowRadioButton->isChecked())
  {
    windowLayout = m_SingleWindowLayouts[mainWindowOrientation];

    if (defaultMultiWindowLayout != WINDOW_LAYOUT_UNKNOWN)
    {
      m_Viewer->SetDefaultMultiWindowLayout(defaultMultiWindowLayout);
    }

    m_ControlsWidget->setEnabled(true);
    m_AxialWindowRadioButton->setEnabled(m_MainWindowOrientation != MIDAS_ORIENTATION_AXIAL);
    m_SagittalWindowRadioButton->setEnabled(m_MainWindowOrientation != MIDAS_ORIENTATION_SAGITTAL);
    m_CoronalWindowRadioButton->setEnabled(m_MainWindowOrientation != MIDAS_ORIENTATION_CORONAL);
  }
  else
  {
    windowLayout = defaultMultiWindowLayout;
  }

  m_LayoutWidget->blockSignals(wasBlocked);

  m_Viewer->SetWindowLayout(windowLayout);
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnFocusChanged()
{
  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  mitk::BaseRenderer* focusedRenderer = focusManager->GetFocused();

  const std::vector<QmitkRenderWindow*>& viewerRenderWindows = m_Viewer->GetRenderWindows();
  for (int i = 0; i < viewerRenderWindows.size(); ++i)
  {
    // If the newly focused window is in this widget, nothing to update. Stop early.
    if (focusedRenderer == viewerRenderWindows[i]->GetRenderer())
    {
      this->OnViewerWindowChanged();
      return;
    }
  }

  mitk::IRenderWindowPart* selectedEditor = this->GetSelectedEditor();
  if (selectedEditor)
  {
    /// Note:
    /// We need to look for the focused window among every window of the editor.
    /// The MITK Display has not got the concept of 'selected' window and always
    /// returns the axial window as 'active'. Therefore we cannot use GetActiveQmitkRenderWindow.
    foreach (QmitkRenderWindow* mainWindow, selectedEditor->GetQmitkRenderWindows().values())
    {
      if (focusedRenderer == mainWindow->GetRenderer())
      {
        this->OnMainWindowChanged(selectedEditor, mainWindow);
        break;
      }
    }
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnViewerWindowChanged()
{
  MIDASOrientation orientation = m_Viewer->GetOrientation();

  if (orientation != MIDAS_ORIENTATION_UNKNOWN)
  {
    int selectedSlice = m_Viewer->GetSelectedSlice(m_Viewer->GetOrientation());
    int maxSlice = m_Viewer->GetMaxSlice(m_Viewer->GetOrientation());

    bool wasBlocked = m_SliceSpinBox->blockSignals(true);
    m_SliceSpinBox->setMaximum(maxSlice);
    m_SliceSpinBox->setValue(selectedSlice);
    m_SliceSpinBox->setEnabled(true);
    m_SliceSpinBox->blockSignals(wasBlocked);

    double magnification = m_Viewer->GetMagnification(m_Viewer->GetOrientation());
    m_Magnification = magnification;

    wasBlocked = m_MagnificationSpinBox->blockSignals(true);
    m_MagnificationSpinBox->setValue(magnification);
    m_MagnificationSpinBox->setEnabled(true);
    m_MagnificationSpinBox->blockSignals(wasBlocked);
  }
  else
  {
    bool wasBlocked = m_SliceSpinBox->blockSignals(true);
    m_SliceSpinBox->setValue(0);
    m_SliceSpinBox->setEnabled(false);
    m_SliceSpinBox->blockSignals(wasBlocked);

    m_Magnification = 0;

    wasBlocked = m_MagnificationSpinBox->blockSignals(true);
    m_MagnificationSpinBox->setValue(0.0);
    m_MagnificationSpinBox->setEnabled(false);
    m_MagnificationSpinBox->blockSignals(wasBlocked);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnMainWindowChanged(mitk::IRenderWindowPart* renderWindowPart, QmitkRenderWindow* mainWindow)
{
  // Get hold of main windows, using QmitkAbstractView lookup mitkIRenderWindowPart.
  QmitkRenderWindow* mainAxialWindow = renderWindowPart->GetQmitkRenderWindow("axial");
  QmitkRenderWindow* mainSagittalWindow = renderWindowPart->GetQmitkRenderWindow("sagittal");
  QmitkRenderWindow* mainCoronalWindow = renderWindowPart->GetQmitkRenderWindow("coronal");

  if (mainWindow != mainAxialWindow
      && mainWindow != mainSagittalWindow
      && mainWindow != mainCoronalWindow)
  {
    return;
  }

  QmitkRenderWindow* axialWindow = m_Viewer->GetAxialWindow();
  QmitkRenderWindow* sagittalWindow = m_Viewer->GetSagittalWindow();
  QmitkRenderWindow* coronalWindow = m_Viewer->GetCoronalWindow();

  mitk::SliceNavigationController* axialSnc = axialWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* sagittalSnc = sagittalWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* coronalSnc = coronalWindow->GetSliceNavigationController();

  if (m_MainWindow)
  {
    m_MainWindow->GetSliceNavigationController()->Disconnect(this);
  }
  if (m_MainAxialWindow)
  {
    m_MainAxialWindow->disconnect(SIGNAL(destroyed(QObject*)), this, SLOT(OnAMainWindowDestroyed(QObject*)));
    m_MainAxialSnc->Disconnect(axialSnc);
    axialSnc->Disconnect(m_MainAxialSnc);
  }
  if (m_MainSagittalWindow)
  {
    m_MainSagittalWindow->disconnect(SIGNAL(destroyed(QObject*)), this, SLOT(OnAMainWindowDestroyed(QObject*)));
    m_MainSagittalSnc->Disconnect(sagittalSnc);
    sagittalSnc->Disconnect(m_MainSagittalSnc);
  }
  if (m_MainCoronalWindow)
  {
    m_MainCoronalWindow->disconnect(SIGNAL(destroyed(QObject*)), this, SLOT(OnAMainWindowDestroyed(QObject*)));
    m_MainCoronalSnc->Disconnect(coronalSnc);
    coronalSnc->Disconnect(m_MainCoronalSnc);
  }

  if (!mainWindow)
  {
    m_VisibilityTracker->SetTrackedRenderer(0);
    m_Viewer->SetEnabled(false);

    m_TimeGeometry = 0;

    m_MainWindowSnc = 0;

    m_MainAxialWindow = 0;
    m_MainSagittalWindow = 0;
    m_MainCoronalWindow = 0;

    m_MainAxialSnc = 0;
    m_MainSagittalSnc = 0;
    m_MainCoronalSnc = 0;

    m_MainWindowOrientation = MIDAS_ORIENTATION_UNKNOWN;

    m_Viewer->RequestUpdate();

    return;
  }

  if (mainWindow != m_MainWindow)
  {
    m_VisibilityTracker->SetTrackedRenderer(mainWindow->GetRenderer());
    m_VisibilityTracker->NotifyAll();
  }

  m_MainWindow = mainWindow;
  m_MainWindowSnc = mainWindow->GetSliceNavigationController();

  m_MainAxialWindow = mainAxialWindow;
  m_MainSagittalWindow = mainSagittalWindow;
  m_MainCoronalWindow = mainCoronalWindow;

  m_MainAxialSnc = mainAxialWindow->GetSliceNavigationController();
  m_MainSagittalSnc = mainSagittalWindow->GetSliceNavigationController();
  m_MainCoronalSnc = mainCoronalWindow->GetSliceNavigationController();

  MIDASOrientation mainWindowOrientation;
  if (mainWindow == mainAxialWindow)
  {
    mainWindowOrientation = MIDAS_ORIENTATION_AXIAL;
  }
  else if (mainWindow == mainSagittalWindow)
  {
    mainWindowOrientation = MIDAS_ORIENTATION_SAGITTAL;
  }
  else if (mainWindow == mainCoronalWindow)
  {
    mainWindowOrientation = MIDAS_ORIENTATION_CORONAL;
  }
  else
  {
    mainWindowOrientation = MIDAS_ORIENTATION_UNKNOWN;
  }

  mitk::TimeGeometry* timeGeometry = const_cast<mitk::TimeGeometry*>(mainWindow->GetRenderer()->GetTimeWorldGeometry());

  /// Note:
  /// The SetWindowLayout function does not change the layout if the viewer has not got
  /// a valid geometry. Therefore, if the viewer is initialised with a geometry for the
  /// first time, we need to set the window layout again, according to the main window
  /// orientation.
  bool geometryFirstInitialised = false;
  if (timeGeometry != m_TimeGeometry)
  {
    if (!m_TimeGeometry)
    {
      geometryFirstInitialised = true;
      m_Viewer->SetEnabled(true);
    }

    if (timeGeometry)
    {
      m_Viewer->SetTimeGeometry(timeGeometry);
    }
    else
    {
      m_Viewer->SetEnabled(false);
    }

    m_TimeGeometry = timeGeometry;
  }

  if (mainWindowOrientation != m_MainWindowOrientation || geometryFirstInitialised)
  {
    m_MainWindowOrientation = mainWindowOrientation;
    this->OnMainWindowOrientationChanged(mainWindowOrientation);
  }

  if (m_MainWindow)
  {
    m_MainWindow->GetSliceNavigationController()->ConnectGeometrySendEvent(this);
  }
  if (m_MainAxialWindow)
  {
    m_MainAxialSnc->ConnectGeometryEvents(axialSnc);
    axialSnc->ConnectGeometryEvents(m_MainAxialSnc);
    this->connect(m_MainAxialWindow, SIGNAL(destroyed(QObject*)), SLOT(OnAMainWindowDestroyed(QObject*)));
  }
  if (mainSagittalWindow)
  {
    m_MainSagittalSnc->ConnectGeometryEvents(sagittalSnc);
    sagittalSnc->ConnectGeometryEvents(m_MainSagittalSnc);
    this->connect(m_MainSagittalWindow, SIGNAL(destroyed(QObject*)), SLOT(OnAMainWindowDestroyed(QObject*)));
  }
  if (mainCoronalWindow)
  {
    m_MainCoronalSnc->ConnectGeometryEvents(coronalSnc);
    coronalSnc->ConnectGeometryEvents(m_MainCoronalSnc);
    this->connect(m_MainCoronalWindow, SIGNAL(destroyed(QObject*)), SLOT(OnAMainWindowDestroyed(QObject*)));
  }

  /// Note that changing the window layout resets the geometry, what sets the selected position in the centre.
  /// Therefore, we resend the main window position here.
  m_MainAxialWindow->GetSliceNavigationController()->SendSlice();
  m_MainSagittalWindow->GetSliceNavigationController()->SendSlice();
  m_MainCoronalWindow->GetSliceNavigationController()->SendSlice();

  m_Viewer->RequestUpdate();
}


//-----------------------------------------------------------------------------
mitk::IRenderWindowPart* QmitkSideViewerWidget::GetSelectedEditor()
{
  berry::IWorkbenchPage::Pointer page = m_ContainingView->GetSite()->GetPage();

  // Returns the active editor if it implements mitk::IRenderWindowPart
  mitk::IRenderWindowPart* renderWindowPart =
      dynamic_cast<mitk::IRenderWindowPart*>(page->GetActiveEditor().GetPointer());

  if (!renderWindowPart)
  {
    // No suitable active editor found, check visible editors
    std::list<berry::IEditorReference::Pointer> editors = page->GetEditorReferences();
    std::list<berry::IEditorReference::Pointer>::iterator editorsIt = editors.begin();
    std::list<berry::IEditorReference::Pointer>::iterator editorsEnd = editors.end();
    for ( ; editorsIt != editorsEnd; ++editorsIt)
    {
      berry::IWorkbenchPart::Pointer part = (*editorsIt)->GetPart(false);
      if (page->IsPartVisible(part))
      {
        renderWindowPart = dynamic_cast<mitk::IRenderWindowPart*>(part.GetPointer());
        break;
      }
    }
  }

  return renderWindowPart;
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnSelectedPositionChanged(niftkSingleViewerWidget* viewer, const mitk::Point3D& selectedPosition)
{
  MIDASOrientation orientation = m_Viewer->GetOrientation();
  if (orientation != MIDAS_ORIENTATION_UNKNOWN)
  {
    bool wasBlocked = m_SliceSpinBox->blockSignals(true);
    m_SliceSpinBox->setValue(m_Viewer->GetSelectedSlice(orientation));
    m_SliceSpinBox->blockSignals(wasBlocked);
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnScaleFactorChanged(niftkSingleViewerWidget*, MIDASOrientation orientation, double scaleFactor)
{
  double magnification = m_Viewer->GetMagnification(m_Viewer->GetOrientation());

  bool wasBlocked = m_MagnificationSpinBox->blockSignals(true);
  m_MagnificationSpinBox->setValue(magnification);
  m_MagnificationSpinBox->blockSignals(wasBlocked);

  m_Magnification = magnification;
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnWindowLayoutChanged(niftkSingleViewerWidget*, WindowLayout windowLayout)
{
  bool axialWindowRadioButtonWasBlocked = m_AxialWindowRadioButton->blockSignals(true);
  bool sagittalWindowRadioButtonWasBlocked = m_SagittalWindowRadioButton->blockSignals(true);
  bool coronalWindowRadioButtonWasBlocked = m_CoronalWindowRadioButton->blockSignals(true);
  bool multiWindowRadioButtonWasBlocked = m_MultiWindowRadioButton->blockSignals(true);
  bool multiWindowComboBoxWasBlocked = m_MultiWindowComboBox->blockSignals(true);

  if (windowLayout == WINDOW_LAYOUT_AXIAL)
  {
    m_AxialWindowRadioButton->setChecked(true);
  }
  else if (windowLayout == WINDOW_LAYOUT_SAGITTAL)
  {
    m_SagittalWindowRadioButton->setChecked(true);
  }
  else if (windowLayout == WINDOW_LAYOUT_CORONAL)
  {
    m_CoronalWindowRadioButton->setChecked(true);
  }
  else if (::IsMultiWindowLayout(windowLayout))
  {
    m_MultiWindowRadioButton->setChecked(true);
    if (windowLayout == WINDOW_LAYOUT_COR_AX_H
        || windowLayout == WINDOW_LAYOUT_COR_SAG_H
        || windowLayout == WINDOW_LAYOUT_SAG_AX_H)
    {
      m_MultiWindowComboBox->setCurrentIndex(0);
    }
    else if (windowLayout == WINDOW_LAYOUT_COR_AX_V
        || windowLayout == WINDOW_LAYOUT_COR_SAG_V
        || windowLayout == WINDOW_LAYOUT_SAG_AX_V)
    {
      m_MultiWindowComboBox->setCurrentIndex(1);
    }
    else if (windowLayout == WINDOW_LAYOUT_ORTHO)
    {
      m_MultiWindowComboBox->setCurrentIndex(2);
    }
  }
  else
  {
    m_AxialWindowRadioButton->setChecked(false);
    m_SagittalWindowRadioButton->setChecked(false);
    m_CoronalWindowRadioButton->setChecked(false);
    m_MultiWindowRadioButton->setChecked(false);
  }

  m_AxialWindowRadioButton->blockSignals(axialWindowRadioButtonWasBlocked);
  m_SagittalWindowRadioButton->blockSignals(sagittalWindowRadioButtonWasBlocked);
  m_CoronalWindowRadioButton->blockSignals(coronalWindowRadioButtonWasBlocked);
  m_MultiWindowRadioButton->blockSignals(multiWindowRadioButtonWasBlocked);
  m_MultiWindowComboBox->blockSignals(multiWindowComboBoxWasBlocked);

  m_WindowLayout = windowLayout;

  /// Note:
  /// The selected window has not necessarily been changed, but it is not costly
  /// to refresh the GUI buttons every time.
//  this->OnViewerWindowChanged();
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnSliceSpinBoxValueChanged(int slice)
{
  bool wasBlocked = m_Viewer->blockSignals(true);
  m_Viewer->SetSelectedSlice(m_Viewer->GetOrientation(), slice);
  m_Viewer->blockSignals(wasBlocked);
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::OnMagnificationSpinBoxValueChanged(double magnification)
{
  double roundedMagnification = std::floor(magnification);

  // If we are between two integers, we raise a new event:
  if (magnification != roundedMagnification)
  {
    double newMagnification = roundedMagnification;
    // If the value has decreased, we have to increase the rounded value.
    if (magnification < m_Magnification)
    {
      newMagnification += 1.0;
    }

    m_MagnificationSpinBox->setValue(newMagnification);
  }
  else
  {
    bool wasBlocked = m_Viewer->blockSignals(true);
    m_Viewer->SetMagnification(m_Viewer->GetOrientation(), magnification);
    m_Viewer->blockSignals(wasBlocked);
    m_Magnification = magnification;
  }
}


//-----------------------------------------------------------------------------
void QmitkSideViewerWidget::SetGeometry(const itk::EventObject& geometrySendEvent)
{
  const mitk::SliceNavigationController::GeometrySendEvent* sendEvent =
      dynamic_cast<const mitk::SliceNavigationController::GeometrySendEvent *>(&geometrySendEvent);

  assert(sendEvent);

  const mitk::TimeGeometry* timeGeometry = sendEvent->GetTimeGeometry();

  assert(timeGeometry);

  if (timeGeometry != m_TimeGeometry)
  {
    m_Viewer->SetTimeGeometry(timeGeometry);

    if (!m_TimeGeometry)
    {
      m_Viewer->SetEnabled(true);

      /// Note: SetWindowLayout does not work when the viewer has no geometry.
      /// Therefore, we need to set it at the first time when the viewer receives
      /// a geometry.
      this->OnMainWindowOrientationChanged(m_MainWindowOrientation);
    }

    m_TimeGeometry = timeGeometry;
  }
}