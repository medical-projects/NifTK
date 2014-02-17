/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkSingleViewerWidgetTest.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <mitkGlobalInteraction.h>
#include <mitkIOUtil.h>
#include <mitkStandaloneDataStorage.h>
#include <mitkTestingMacros.h>

#include <QmitkRenderingManagerFactory.h>
#include <QmitkApplicationCursor.h>

#include <mitkMIDASOrientationUtils.h>
#include <mitkNifTKCoreObjectFactory.h>
#include <niftkSingleViewerWidget.h>
#include <niftkMultiViewerVisibilityManager.h>

#include <mitkItkSignalCollector.cxx>
#include <mitkQtSignalCollector.cxx>
#include <niftkSingleViewerWidgetState.h>


class niftkSingleViewerWidgetTestClassPrivate
{
public:

  niftkSingleViewerWidgetTestClassPrivate()
  : GeometrySendEvent(NULL, 0)
  , GeometryUpdateEvent(NULL, 0)
  , GeometryTimeEvent(NULL, 0)
  , GeometrySliceEvent(NULL, 0)
  {
  }

  std::string FileName;
  mitk::DataStorage::Pointer DataStorage;
  mitk::RenderingManager::Pointer RenderingManager;

  mitk::DataNode::Pointer ImageNode;
  mitk::Image* Image;
  mitk::Vector3D ExtentsInVxInWorldCoordinateOrder;
  mitk::Vector3D SpacingsInWorld;
  int UpDirectionsInWorld[3];

  niftkSingleViewerWidget* Viewer;
  niftkMultiViewerVisibilityManager* VisibilityManager;

  niftkSingleViewerWidgetTestClass::ViewerStateTester::Pointer StateTester;

  bool InteractiveMode;

  mitk::FocusEvent FocusEvent;
  mitk::SliceNavigationController::GeometrySliceEvent GeometrySendEvent;
  mitk::SliceNavigationController::GeometrySliceEvent GeometryUpdateEvent;
  mitk::SliceNavigationController::GeometrySliceEvent GeometryTimeEvent;
  mitk::SliceNavigationController::GeometrySliceEvent GeometrySliceEvent;

  const char* GeometryChanged;
  const char* WindowLayoutChanged;
  const char* SelectedRenderWindowChanged;
  const char* SelectedTimeStepChanged;
  const char* SelectedPositionChanged;
  const char* CursorPositionChanged;
  const char* ScaleFactorChanged;
  const char* CursorPositionBindingChanged;
  const char* ScaleFactorBindingChanged;
  const char* CursorVisibilityChanged;
};


// --------------------------------------------------------------------------
niftkSingleViewerWidgetTestClass::niftkSingleViewerWidgetTestClass()
: QObject()
, d_ptr(new niftkSingleViewerWidgetTestClassPrivate())
{
  Q_D(niftkSingleViewerWidgetTestClass);

  d->ImageNode = 0;
  d->Image = 0;
  d->ExtentsInVxInWorldCoordinateOrder.Fill(0.0);
  d->SpacingsInWorld.Fill(1.0);
  d->UpDirectionsInWorld[0] = 0;
  d->UpDirectionsInWorld[1] = 0;
  d->UpDirectionsInWorld[2] = 0;
  d->Viewer = 0;
  d->VisibilityManager = 0;
  d->InteractiveMode = false;

  d->SelectedRenderWindowChanged = SIGNAL(SelectedRenderWindowChanged(MIDASOrientation));
  d->SelectedPositionChanged = SIGNAL(SelectedPositionChanged(niftkSingleViewerWidget*, const mitk::Point3D&));
  d->SelectedTimeStepChanged = SIGNAL(SelectedTimeStepChanged(niftkSingleViewerWidget*, int));
  d->CursorPositionChanged = SIGNAL(CursorPositionChanged(niftkSingleViewerWidget*, MIDASOrientation, const mitk::Vector2D&));
  d->ScaleFactorChanged = SIGNAL(ScaleFactorChanged(niftkSingleViewerWidget*, MIDASOrientation, double));
  d->CursorPositionBindingChanged = SIGNAL(CursorPositionBindingChanged(niftkSingleViewerWidget*, bool));
  d->ScaleFactorBindingChanged = SIGNAL(ScaleFactorBindingChanged(niftkSingleViewerWidget*, bool));
  d->WindowLayoutChanged = SIGNAL(WindowLayoutChanged(niftkSingleViewerWidget*, WindowLayout));
  d->GeometryChanged = SIGNAL(GeometryChanged(niftkSingleViewerWidget*, mitk::TimeGeometry*));
  d->CursorVisibilityChanged = SIGNAL(CursorVisibilityChanged(niftkSingleViewerWidget*, bool));
}


// --------------------------------------------------------------------------
niftkSingleViewerWidgetTestClass::~niftkSingleViewerWidgetTestClass()
{
}


// --------------------------------------------------------------------------
std::string niftkSingleViewerWidgetTestClass::GetFileName() const
{
  Q_D(const niftkSingleViewerWidgetTestClass);
  return d->FileName;
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::SetFileName(const std::string& fileName)
{
  Q_D(niftkSingleViewerWidgetTestClass);
  d->FileName = fileName;
}


// --------------------------------------------------------------------------
bool niftkSingleViewerWidgetTestClass::GetInteractiveMode() const
{
  Q_D(const niftkSingleViewerWidgetTestClass);
  return d->InteractiveMode;
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::SetInteractiveMode(bool interactiveMode)
{
  Q_D(niftkSingleViewerWidgetTestClass);
  d->InteractiveMode = interactiveMode;
}


// --------------------------------------------------------------------------
QPoint niftkSingleViewerWidgetTestClass::GetPointAtCursorPosition(QmitkRenderWindow *renderWindow, const mitk::Vector2D& cursorPosition)
{
  QRect rect = renderWindow->rect();
  double x = cursorPosition[0] * rect.width();
  double y = (1.0 - cursorPosition[1]) * rect.height();
  return QPoint(x, y);
}


// --------------------------------------------------------------------------
mitk::Vector2D niftkSingleViewerWidgetTestClass::GetCursorPositionAtPoint(QmitkRenderWindow *renderWindow, const QPoint& point)
{
  QRect rect = renderWindow->rect();
  mitk::Vector2D cursorPosition;
  cursorPosition[0] = double(point.x()) / rect.width();
  cursorPosition[1] = 1.0 - double(point.y()) / rect.height();
  return cursorPosition;
}


// --------------------------------------------------------------------------
bool niftkSingleViewerWidgetTestClass::Equals(const mitk::Point3D& selectedPosition1, const mitk::Point3D& selectedPosition2)
{
  Q_D(niftkSingleViewerWidgetTestClass);

  for (int i = 0; i < 3; ++i)
  {
    double tolerance = d->SpacingsInWorld[i] / 2.0;
    if (std::abs(selectedPosition1[i] - selectedPosition2[i]) > tolerance)
    {
      return false;
    }
  }

  return true;
}


// --------------------------------------------------------------------------
bool niftkSingleViewerWidgetTestClass::Equals(const mitk::Vector2D& cursorPosition1, const mitk::Vector2D& cursorPosition2, double tolerance)
{
  return std::abs(cursorPosition1[0] - cursorPosition2[0]) <= tolerance && std::abs(cursorPosition1[1] - cursorPosition2[1]) <= tolerance;
}


// --------------------------------------------------------------------------
bool niftkSingleViewerWidgetTestClass::Equals(const std::vector<mitk::Vector2D>& cursorPositions1, const std::vector<mitk::Vector2D>& cursorPositions2, double tolerance)
{
  return cursorPositions1.size() == std::size_t(3)
      && cursorPositions2.size() == std::size_t(3)
      && Self::Equals(cursorPositions1[0], cursorPositions2[0], tolerance)
      && Self::Equals(cursorPositions1[1], cursorPositions2[1], tolerance)
      && Self::Equals(cursorPositions1[2], cursorPositions2[2], tolerance);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::initTestCase()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  // Need to load images, specifically using MIDAS/DRC object factory.
  ::RegisterNifTKCoreObjectFactory();

  mitk::GlobalInteraction* globalInteraction =  mitk::GlobalInteraction::GetInstance();
  globalInteraction->Initialize("global");
  globalInteraction->GetStateMachineFactory()->LoadBehaviorString(mitk::DnDDisplayStateMachine::STATE_MACHINE_XML);

  /// Create and register RenderingManagerFactory for this platform.
  static QmitkRenderingManagerFactory qmitkRenderingManagerFactory;
  Q_UNUSED(qmitkRenderingManagerFactory);

  /// Create one instance
  static QmitkApplicationCursor globalQmitkApplicationCursor;
  Q_UNUSED(globalQmitkApplicationCursor);

  d->DataStorage = mitk::StandaloneDataStorage::New();

  d->RenderingManager = mitk::RenderingManager::GetInstance();
  d->RenderingManager->SetDataStorage(d->DataStorage);

  /// Disable VTK warnings. For some reason, they appear using these tests, but
  /// not with the real application. We simply suppress them here.
  vtkObject::GlobalWarningDisplayOff();

  std::vector<std::string> files;
  files.push_back(d->FileName);

  mitk::IOUtil::LoadFiles(files, *(d->DataStorage.GetPointer()));
  mitk::DataStorage::SetOfObjects::ConstPointer allImages = d->DataStorage->GetAll();
  MITK_TEST_CONDITION_REQUIRED(mitk::Equal(allImages->size(), 1), ".. Test image loaded.");

  d->ImageNode = (*allImages)[0];

  d->VisibilityManager = new niftkMultiViewerVisibilityManager(d->DataStorage);
  d->VisibilityManager->SetInterpolationType(DNDDISPLAY_CUBIC_INTERPOLATION);
  d->VisibilityManager->SetDefaultWindowLayout(WINDOW_LAYOUT_CORONAL);
  d->VisibilityManager->SetDropType(DNDDISPLAY_DROP_SINGLE);

  d->Image = dynamic_cast<mitk::Image*>(d->ImageNode->GetData());
  mitk::GetExtentsInVxInWorldCoordinateOrder(d->Image, d->ExtentsInVxInWorldCoordinateOrder);
  mitk::GetSpacingInWorldCoordinateOrder(d->Image, d->SpacingsInWorld);
  /// TODO:
  /// The right image up directions are hard-coded to match the test image,
  /// because the mitk::GetUpDirection function returned incorrect values.
//  d->UpDirectionsInWorld[SagittalAxis] = mitk::GetUpDirection(d->Image, MIDAS_ORIENTATION_SAGITTAL);
//  d->UpDirectionsInWorld[CoronalAxis] = mitk::GetUpDirection(d->Image, MIDAS_ORIENTATION_CORONAL);
//  d->UpDirectionsInWorld[AxialAxis] = mitk::GetUpDirection(d->Image, MIDAS_ORIENTATION_AXIAL);
  d->UpDirectionsInWorld[SagittalAxis] = +1;
  d->UpDirectionsInWorld[CoronalAxis] = -1;
  d->UpDirectionsInWorld[AxialAxis] = -1;
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::cleanupTestCase()
{
  Q_D(niftkSingleViewerWidgetTestClass);
  delete d->VisibilityManager;
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::init()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  d->Viewer = new niftkSingleViewerWidget(0, d->RenderingManager);
  d->Viewer->SetDataStorage(d->DataStorage);
  d->Viewer->setObjectName(tr("niftkSingleViewerWidget"));

//  QColor backgroundColour("black");
  QColor backgroundColour("#fffaf0");
  d->Viewer->SetDirectionAnnotationsVisible(true);
  d->Viewer->SetBackgroundColor(backgroundColour);
  d->Viewer->SetShow3DWindowIn2x2WindowLayout(true);
  d->Viewer->SetRememberSettingsPerWindowLayout(false);
  d->Viewer->SetDisplayInteractionsEnabled(true);
  d->Viewer->SetCursorPositionBinding(true);
  d->Viewer->SetScaleFactorBinding(true);
  d->Viewer->SetDefaultSingleWindowLayout(WINDOW_LAYOUT_CORONAL);
  d->Viewer->SetDefaultMultiWindowLayout(WINDOW_LAYOUT_ORTHO);

//  d->VisibilityManager->connect(d->Viewer, SIGNAL(NodesDropped(niftkSingleViewerWidget*, QmitkRenderWindow*, std::vector<mitk::DataNode*>)), SLOT(OnNodesDropped(niftkSingleViewerWidget*, QmitkRenderWindow*, std::vector<mitk::DataNode*>)), Qt::DirectConnection);

  d->VisibilityManager->RegisterViewer(d->Viewer);
  d->VisibilityManager->SetAllNodeVisibilityForViewer(0, false);

  d->Viewer->resize(1024, 1024);
  d->Viewer->show();

  QTest::qWaitForWindowShown(d->Viewer);

  std::vector<mitk::DataNode*> nodes(1);
  nodes[0] = d->ImageNode;

  QmitkRenderWindow* axialWindow = d->Viewer->GetAxialWindow();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();
  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();

  this->DropNodes(coronalWindow, nodes);

  d->Viewer->SetCursorVisible(true);

  /// Create a state tester that works for all of the test functions.

  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();
  mitk::SliceNavigationController* axialSnc = axialWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* sagittalSnc = sagittalWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* coronalSnc = coronalWindow->GetSliceNavigationController();

  d->StateTester = ViewerStateTester::New(d->Viewer);

  d->StateTester->Connect(focusManager, mitk::FocusEvent());
  mitk::SliceNavigationController::GeometrySliceEvent geometrySliceEvent(NULL, 0);
  d->StateTester->Connect(axialSnc, geometrySliceEvent);
  d->StateTester->Connect(sagittalSnc, geometrySliceEvent);
  d->StateTester->Connect(coronalSnc, geometrySliceEvent);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::cleanup()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// Release the pointer so that the desctructor is be called.
  d->StateTester = 0;

  if (d->InteractiveMode)
  {
    QEventLoop loop;
    loop.connect(d->Viewer, SIGNAL(destroyed()), SLOT(quit()));
    loop.exec();
  }

  d->VisibilityManager->DeRegisterAllViewers();

  delete d->Viewer;
  d->Viewer = 0;
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::DropNodes(QmitkRenderWindow* renderWindow, const std::vector<mitk::DataNode*>& nodes)
{
  Q_D(niftkSingleViewerWidgetTestClass);
/*
  QMimeData* mimeData = new QMimeData;
  QString dataNodeAddresses("");
  for (int i = 0; i < nodes.size(); ++i)
  {
    long dataNodeAddress = reinterpret_cast<long>(nodes[i]);
    QTextStream(&dataNodeAddresses) << dataNodeAddress;

    if (i != nodes.size() - 1)
    {
      QTextStream(&dataNodeAddresses) << ",";
    }
  }
  mimeData->setData("application/x-mitk-datanodes", QByteArray(dataNodeAddresses.toAscii()));
  QStringList types;
  types << "application/x-mitk-datanodes";
  QDropEvent dropEvent(renderWindow->rect().center(), Qt::CopyAction | Qt::MoveAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  dropEvent.acceptProposedAction();
  QApplication::instance()->sendEvent(renderWindow, &dropEvent);
*/
  d->VisibilityManager->OnNodesDropped(d->Viewer, renderWindow, nodes);
  d->Viewer->OnNodesDropped(renderWindow, nodes);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testViewer()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// Tests if the viewer has been successfully created.
  QVERIFY(d->Viewer);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetOrientation()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  ViewerState::Pointer state = ViewerState::New(d->Viewer);
  d->StateTester->SetExpectedState(state);

  MIDASOrientation orientation = d->Viewer->GetOrientation();

  /// The default window layout was set to coronal in the init() function.
  QCOMPARE(orientation, MIDAS_ORIENTATION_CORONAL);
  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetSelectedPosition()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// Make sure that the state does not change and no signal is sent out.
  ViewerState::Pointer state = ViewerState::New(d->Viewer);
  d->StateTester->SetExpectedState(state);

  mitk::Point3D selectedPosition = d->Viewer->GetSelectedPosition();

  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());

  mitk::Point3D centre = d->Image->GetGeometry()->GetCenter();

  for (int i = 0; i < 3; ++i)
  {
    double distanceFromCentre = std::abs(selectedPosition[i] - centre[i]);
    if (static_cast<int>(d->ExtentsInVxInWorldCoordinateOrder[i]) % 2 == 0)
    {
      /// If the number of slices is an even number then the selected position
      /// must be a half voxel far from the centre, either way.
      /// Tolerance is 0.001 millimetre because of float precision.
      QVERIFY(std::abs(distanceFromCentre - d->SpacingsInWorld[i] / 2.0) < 0.001);
    }
    else
    {
      /// If the number of slices is an odd number then the selected position
      /// must be exactly at the centre position.
      /// Tolerance is 0.001 millimetre because of float precision.
      QVERIFY(distanceFromCentre < 0.001);
    }
  }
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSetSelectedPosition()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  QmitkRenderWindow* axialWindow = d->Viewer->GetAxialWindow();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();
  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();

  /// Register to listen to SliceNavigators, slice changed events.
  mitk::SliceNavigationController* axialSnc = axialWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* sagittalSnc = sagittalWindow->GetSliceNavigationController();
  mitk::SliceNavigationController* coronalSnc = coronalWindow->GetSliceNavigationController();

  ViewerState::Pointer expectedState = ViewerState::New(d->Viewer);
  unsigned expectedAxialSlice = d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL);
  unsigned expectedSagittalSlice = d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL);
  unsigned expectedCoronalSlice = d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL);

  mitk::Point3D selectedPosition = d->Viewer->GetSelectedPosition();

  selectedPosition[SagittalAxis] += 20 * d->SpacingsInWorld[SagittalAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedSagittalSlice += d->UpDirectionsInWorld[SagittalAxis] * 20;
  std::vector<mitk::Vector2D> cursorPositions = expectedState->GetCursorPositions();
  std::vector<double> scaleFactors = expectedState->GetScaleFactors();
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] += 20 * d->SpacingsInWorld[SagittalAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->width();
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();

  selectedPosition[CoronalAxis] += 20 * d->SpacingsInWorld[CoronalAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedCoronalSlice += d->UpDirectionsInWorld[CoronalAxis] * 20;
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();

  selectedPosition[AxialAxis] += 20 * d->SpacingsInWorld[AxialAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedAxialSlice += d->UpDirectionsInWorld[AxialAxis] * 20;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] += 20 * d->SpacingsInWorld[AxialAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->height();
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();

  selectedPosition[SagittalAxis] -= 30 * d->SpacingsInWorld[SagittalAxis];
  selectedPosition[CoronalAxis] -= 30 * d->SpacingsInWorld[CoronalAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedSagittalSlice -= d->UpDirectionsInWorld[SagittalAxis] * 30;
  expectedCoronalSlice -= d->UpDirectionsInWorld[CoronalAxis] * 30;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] -= 30 * d->SpacingsInWorld[SagittalAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->width();
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();

  selectedPosition[SagittalAxis] -= 40 * d->SpacingsInWorld[SagittalAxis];
  selectedPosition[AxialAxis] -= 40 * d->SpacingsInWorld[AxialAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedSagittalSlice -= d->UpDirectionsInWorld[SagittalAxis] * 40;
  expectedAxialSlice -= d->UpDirectionsInWorld[AxialAxis] * 40;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] -= 40 * d->SpacingsInWorld[SagittalAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->width();
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] -= 40 * d->SpacingsInWorld[AxialAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->height();
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);

//  MITK_INFO << "actual selected position: " << selectedPosition;
//  MITK_INFO << "actual slices: " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL) << " " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL) << " " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();

  selectedPosition = d->Viewer->GetSelectedPosition();
  selectedPosition[CoronalAxis] += 50 * d->SpacingsInWorld[CoronalAxis];
  selectedPosition[AxialAxis] += 50 * d->SpacingsInWorld[AxialAxis];
  expectedState->SetSelectedPosition(selectedPosition);
  expectedCoronalSlice += d->UpDirectionsInWorld[CoronalAxis] * 50;
  expectedAxialSlice += d->UpDirectionsInWorld[AxialAxis] * 50;
//  MITK_INFO << "delta (sag, cor, ax): 0, +50, +50";
//  MITK_INFO << "expected selected position: " << selectedPosition;
//  MITK_INFO << "expected slices: " << expectedSagittalSlice << " " << expectedCoronalSlice << " " << expectedAxialSlice;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] += 50 * d->SpacingsInWorld[AxialAxis] / scaleFactors[MIDAS_ORIENTATION_CORONAL] / coronalWindow->height();
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedPosition(selectedPosition);
//  MITK_INFO << "actual selected position: " << selectedPosition;
//  MITK_INFO << "actual slices: " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL) << " " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL) << " " << d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL);

  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_AXIAL), expectedAxialSlice);
  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_SAGITTAL), expectedSagittalSlice);
  /// TODO:
  /// This test fails and is temporarily disabled.
//  QCOMPARE(d->Viewer->GetSliceIndex(MIDAS_ORIENTATION_CORONAL), expectedCoronalSlice);
  QCOMPARE(d->Viewer->GetSelectedPosition(), selectedPosition);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(2));

  d->StateTester->Clear();
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetCursorPosition()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// Note that the cursor positions of a render window are first initialised
  /// when the render window gets visible.

  mitk::Vector2D centrePosition;
  centrePosition.Fill(0.5);

  mitk::Vector2D cursorPosition = d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL);

  QVERIFY(::EqualsWithTolerance(cursorPosition, centrePosition));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(ViewerState::New(d->Viewer));

  cursorPosition = d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL);

  QVERIFY(::EqualsWithTolerance(cursorPosition, centrePosition));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(ViewerState::New(d->Viewer));

  cursorPosition = d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL);

  QVERIFY(::EqualsWithTolerance(cursorPosition, centrePosition));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSetCursorPosition()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  ViewerState::Pointer expectedState = ViewerState::New(d->Viewer);

  std::vector<mitk::Vector2D> cursorPositions = d->Viewer->GetCursorPositions();
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] = 0.4;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] = 0.6;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPosition(MIDAS_ORIENTATION_CORONAL, cursorPositions[MIDAS_ORIENTATION_CORONAL]);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals(d->CursorPositionChanged).size() == 1);
  QVERIFY(d->StateTester->GetQtSignals().size() == 1);

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_AXIAL][0] = 0.45;
  cursorPositions[MIDAS_ORIENTATION_AXIAL][1] = 0.65;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPosition(MIDAS_ORIENTATION_AXIAL, cursorPositions[MIDAS_ORIENTATION_AXIAL]);

  QVERIFY(Self::Equals(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), cursorPositions[MIDAS_ORIENTATION_AXIAL]));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][0] = 0.35;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][1] = 0.65;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPosition(MIDAS_ORIENTATION_SAGITTAL, cursorPositions[MIDAS_ORIENTATION_SAGITTAL]);

  QVERIFY(Self::Equals(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), cursorPositions[MIDAS_ORIENTATION_SAGITTAL]));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QVERIFY(d->StateTester->GetQtSignals().empty());
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetCursorPositions()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// Note that the cursor positions of a render window are first initialised
  /// when the render window gets visible.

  mitk::Vector2D centrePosition;
  centrePosition.Fill(0.5);
  std::vector<mitk::Vector2D> centrePositions(3);
  std::fill(centrePositions.begin(), centrePositions.end(), centrePosition);

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);
  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);
  d->StateTester->Clear();

  QVERIFY(ViewerState::New(d->Viewer)->EqualsWithTolerance(d->Viewer->GetCursorPositions(), centrePositions));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  QVERIFY(d->StateTester->GetQtSignals().empty());
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSetCursorPositions()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  ViewerState::Pointer expectedState = ViewerState::New(d->Viewer);

  std::vector<mitk::Vector2D> cursorPositions = d->Viewer->GetCursorPositions();
  cursorPositions[MIDAS_ORIENTATION_AXIAL][0] = 0.41;
  cursorPositions[MIDAS_ORIENTATION_AXIAL][1] = 0.61;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(Self::Equals(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), cursorPositions[MIDAS_ORIENTATION_AXIAL]));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][0] = 0.52;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][1] = 0.72;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(Self::Equals(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), cursorPositions[MIDAS_ORIENTATION_SAGITTAL]));
  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] = 0.33;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] = 0.23;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_AXIAL][0] = 0.44;
  cursorPositions[MIDAS_ORIENTATION_AXIAL][1] = 0.74;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][0] = 0.64;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][1] = 0.84;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QVERIFY(d->StateTester->GetQtSignals().empty());

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_AXIAL][0] = 0.25;
  cursorPositions[MIDAS_ORIENTATION_AXIAL][1] = 0.35;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] = 0.75;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] = 0.95;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][0] = 0.16;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][1] = 0.56;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] = 0.46;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] = 0.86;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));

  d->StateTester->Clear();

  cursorPositions[MIDAS_ORIENTATION_AXIAL][0] = 0.27;
  cursorPositions[MIDAS_ORIENTATION_AXIAL][1] = 0.37;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][0] = 0.47;
  cursorPositions[MIDAS_ORIENTATION_SAGITTAL][1] = 0.57;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][0] = 0.67;
  cursorPositions[MIDAS_ORIENTATION_CORONAL][1] = 0.77;
  expectedState->SetCursorPositions(cursorPositions);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetCursorPositions(cursorPositions);

  QVERIFY(d->StateTester->GetItkSignals().empty());
  /// Note that the CursorPositionChanged and ScaleFactorChanged signals are emitted only for the visible windows.
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetWindowLayout()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  /// The default window layout was set to coronal in the init() function.
  QCOMPARE(d->Viewer->GetWindowLayout(), WINDOW_LAYOUT_CORONAL);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testGetSelectedRenderWindow()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();

  /// The default window layout was set to coronal in the init() function.
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), coronalWindow);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSetSelectedRenderWindow()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();

  QmitkRenderWindow* axialWindow = d->Viewer->GetAxialWindow();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();
  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();

  QVERIFY(coronalWindow->hasFocus());
  QCOMPARE(focusManager->GetFocused(), coronalWindow->GetRenderer());
  QCOMPARE(d->Viewer->IsSelected(), true);

  d->StateTester->Clear();

  ViewerState::Pointer expectedState = ViewerState::New(d->Viewer);
  expectedState->SetSelectedRenderWindow(axialWindow);
  expectedState->SetOrientation(MIDAS_ORIENTATION_AXIAL);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetSelectedRenderWindow(axialWindow);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QCOMPARE(d->Viewer->IsSelected(), true);

  QCOMPARE(d->StateTester->GetItkSignals(d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSetWindowLayout()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  mitk::FocusManager* focusManager = mitk::GlobalInteraction::GetInstance()->GetFocusManager();

  QmitkRenderWindow* axialWindow = d->Viewer->GetAxialWindow();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();
  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();
  QmitkRenderWindow* _3DWindow = d->Viewer->Get3DWindow();

  mitk::Point3D centreWorldPosition = d->Viewer->GetGeometry()->GetCenterInWorld();

  mitk::Vector2D centreDisplayPosition;
  centreDisplayPosition.Fill(0.5);

  std::size_t scaleFactorChanges;
  std::size_t cursorPositionChanges;

  QVERIFY(coronalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), coronalWindow);
  QCOMPARE(focusManager->GetFocused(), coronalWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(Self::Equals(d->Viewer->GetSelectedPosition(), centreWorldPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));

  ViewerState::Pointer stateCoronal = ViewerState::New(d->Viewer);

  /// We cannot test the full state because the cursor position and scale factor
  /// of the sagittal and the axial windows have not been initialised yet.
  /// They will be initialised when we first switch to those windows.

  /// The default layout was set to coronal in the init() function.
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(!coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(Self::Equals(d->Viewer->GetSelectedPosition(), centreWorldPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->ScaleFactorChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(3));

  ViewerState::Pointer stateSagittal = ViewerState::New(d->Viewer);

  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), axialWindow);
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(!coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(Self::Equals(d->Viewer->GetSelectedPosition(), centreWorldPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->ScaleFactorChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(3));

  ViewerState::Pointer stateAxial = ViewerState::New(d->Viewer);

  ViewerState::Pointer expectedState = ViewerState::New(d->Viewer);

  d->StateTester->Clear();
  expectedState->SetWindowLayout(WINDOW_LAYOUT_CORONAL);
  expectedState->SetOrientation(MIDAS_ORIENTATION_CORONAL);
  expectedState->SetSelectedRenderWindow(coronalWindow);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);

  QVERIFY(coronalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), coronalWindow);
  QCOMPARE(focusManager->GetFocused(), coronalWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->CursorPositionChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->ScaleFactorChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(3));

  d->StateTester->Clear();
  expectedState->SetWindowLayout(WINDOW_LAYOUT_3D);
  expectedState->SetOrientation(MIDAS_ORIENTATION_UNKNOWN);
  expectedState->SetSelectedRenderWindow(_3DWindow);
  d->StateTester->SetExpectedState(expectedState);

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3D);

  QVERIFY(_3DWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), _3DWindow);
  QCOMPARE(focusManager->GetFocused(), _3DWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(!coronalWindow->isVisible());
  QVERIFY(_3DWindow->isVisible());
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals().size(), std::size_t(1));

  ViewerState::Pointer state3D = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3H);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), axialWindow);
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges == std::size_t(3));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges == std::size_t(3));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 1 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer state3H = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3V);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), axialWindow);
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(3));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(3));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer state3V = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_H);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), axialWindow);
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateCorAxH = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_V);

  QVERIFY(axialWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), axialWindow);
  QCOMPARE(focusManager->GetFocused(), axialWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(!sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateCorAxV = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_H);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(1));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 1 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateCorSagH = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_V);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(!axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateCorSagV = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_H);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(!coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateSagAxH = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_V);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(!coronalWindow->isVisible());
  QVERIFY(!_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(2));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(2));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateSagAxV = ViewerState::New(d->Viewer);

  d->StateTester->Clear();

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_ORTHO);

  QVERIFY(sagittalWindow->hasFocus());
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
  QCOMPARE(focusManager->GetFocused(), sagittalWindow->GetRenderer());
  QVERIFY(axialWindow->isVisible());
  QVERIFY(sagittalWindow->isVisible());
  QVERIFY(coronalWindow->isVisible());
  QVERIFY(_3DWindow->isVisible());
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_AXIAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_SAGITTAL), centreDisplayPosition));
  QVERIFY(::EqualsWithTolerance(d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL), centreDisplayPosition));
  QCOMPARE(d->StateTester->GetItkSignals(focusManager, d->FocusEvent).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals().size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetQtSignals(d->SelectedRenderWindowChanged).size(), std::size_t(0));
  cursorPositionChanges = d->StateTester->GetQtSignals(d->CursorPositionChanged).size();
  QVERIFY(cursorPositionChanges <= std::size_t(3));
  scaleFactorChanges = d->StateTester->GetQtSignals(d->ScaleFactorChanged).size();
  QVERIFY(scaleFactorChanges <= std::size_t(3));
  QCOMPARE(d->StateTester->GetQtSignals().size(), 0 + cursorPositionChanges + scaleFactorChanges);

  ViewerState::Pointer stateOrtho = ViewerState::New(d->Viewer);

  ///
  /// Check if we get the same state when returning to a previously selected orientation.
  ///

  mitk::Point3D randomSelectedPosition;
  mitk::Geometry3D* geometry = d->Image->GetGeometry();
  randomSelectedPosition = geometry->GetOrigin()
      + geometry->GetAxisVector(0) * ((double) std::rand() / RAND_MAX)
      + geometry->GetAxisVector(1) * ((double) std::rand() / RAND_MAX)
      + geometry->GetAxisVector(2) * ((double) std::rand() / RAND_MAX);
  std::vector<mitk::Vector2D> randomCursorPositions(3);
  randomCursorPositions[0][0] = (double) std::rand() / RAND_MAX;
  randomCursorPositions[0][1] = (double) std::rand() / RAND_MAX;
  randomCursorPositions[1][0] = (double) std::rand() / RAND_MAX;
  randomCursorPositions[1][1] = (double) std::rand() / RAND_MAX;
  randomCursorPositions[2][0] = (double) std::rand() / RAND_MAX;
  randomCursorPositions[2][1] = (double) std::rand() / RAND_MAX;
  std::vector<double> randomScaleFactors(3);
  randomScaleFactors[0] = 2 * (double) std::rand() / RAND_MAX;
  randomScaleFactors[1] = 2 * (double) std::rand() / RAND_MAX;
  randomScaleFactors[2] = 2 * (double) std::rand() / RAND_MAX;

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateAxial);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateAxial);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagittal);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagittal);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAGITTAL);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCoronal);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCoronal);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3D);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3D);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3D);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3D);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3H);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3H);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3H);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3H);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3V);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3V);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(state3V);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_3V);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorAxH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_H);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorAxH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_H);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorAxV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_V);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorAxV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_AX_V);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorSagH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_H);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorSagH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_H);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorSagV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_V);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateCorSagV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_COR_SAG_V);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagAxH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_H);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagAxH);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_H);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagAxV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_V);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateSagAxV);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_SAG_AX_V);

  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateOrtho);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_ORTHO);

  d->StateTester->Clear();
  d->Viewer->SetSelectedPosition(randomSelectedPosition);
  d->StateTester->Clear();
  d->Viewer->SetCursorPositions(randomCursorPositions);
  d->StateTester->Clear();
  d->Viewer->SetScaleFactors(randomScaleFactors);
  d->StateTester->Clear();
  d->StateTester->SetExpectedState(stateOrtho);
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_ORTHO);
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testRememberSelectedPosition()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  d->Viewer->SetRememberSettingsPerWindowLayout(true);

  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();

  mitk::Point3D selectedPosition;
  selectedPosition[0] = 100.0;
  selectedPosition[1] = -50.0;
  selectedPosition[2] = -100.0;

  d->Viewer->SetSelectedPosition(selectedPosition);

  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);
  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);

  mitk::Point3D newPosition = d->Viewer->GetSelectedPosition();

  QVERIFY(this->Equals(newPosition, selectedPosition));

  mitk::Vector2D coronalCursorPosition;
  coronalCursorPosition[0] = 0.4;
  coronalCursorPosition[1] = 0.6;
  QPoint pointAtCoronalCursorPosition = this->GetPointAtCursorPosition(coronalWindow, coronalCursorPosition);

  d->StateTester->Clear();
  QTest::mouseClick(coronalWindow, Qt::LeftButton, Qt::NoModifier, pointAtCoronalCursorPosition);

  mitk::Point3D newPosition2 = d->Viewer->GetSelectedPosition();
  mitk::Vector2D newCoronalCursorPosition2 = d->Viewer->GetCursorPosition(MIDAS_ORIENTATION_CORONAL);

  QVERIFY(this->Equals(coronalCursorPosition, newCoronalCursorPosition2));

  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_AXIAL);
  d->StateTester->Clear();
  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_CORONAL);

  mitk::Point3D newPosition3 = d->Viewer->GetSelectedPosition();
  QVERIFY(this->Equals(newPosition3, newPosition2));
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSelectPositionByInteraction()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  QmitkRenderWindow* axialWindow = d->Viewer->GetAxialWindow();
  mitk::SliceNavigationController* axialSnc = axialWindow->GetSliceNavigationController();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();
  mitk::SliceNavigationController* sagittalSnc = sagittalWindow->GetSliceNavigationController();
  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();
  mitk::SliceNavigationController* coronalSnc = coronalWindow->GetSliceNavigationController();

  QPoint centre = coronalWindow->rect().center();
  QTest::mouseClick(coronalWindow, Qt::LeftButton, Qt::NoModifier, centre);

  d->StateTester->Clear();

  mitk::Point3D lastPosition = d->Viewer->GetSelectedPosition();

  QPoint newPoint = centre;
  newPoint.rx() += 30;
  std::vector<mitk::Vector2D> expectedCursorPositions = d->Viewer->GetCursorPositions();
  expectedCursorPositions[MIDAS_ORIENTATION_CORONAL] = Self::GetCursorPositionAtPoint(coronalWindow, newPoint);

  QTest::mouseClick(coronalWindow, Qt::LeftButton, Qt::NoModifier, newPoint);

  mitk::Point3D newPosition = d->Viewer->GetSelectedPosition();
  std::vector<mitk::Vector2D> newCursorPositions = d->Viewer->GetCursorPositions();
  QVERIFY(newPosition[0] != lastPosition[0]);
  QCOMPARE(newPosition[1], lastPosition[1]);
  QCOMPARE(newPosition[2], lastPosition[2]);
  QVERIFY(Self::Equals(newCursorPositions, expectedCursorPositions));
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));

  d->StateTester->Clear();

  lastPosition = newPosition;

  newPoint.ry() += 20;
  QTest::mouseClick(coronalWindow, Qt::LeftButton, Qt::NoModifier, newPoint);

  newPosition = d->Viewer->GetSelectedPosition();
  QCOMPARE(newPosition[0], lastPosition[0]);
  QCOMPARE(newPosition[1], lastPosition[1]);
  QVERIFY(newPosition[2] != lastPosition[1]);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(0));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));

  d->StateTester->Clear();

  lastPosition = d->Viewer->GetSelectedPosition();

  newPoint.rx() -= 40;
  newPoint.ry() += 50;
  QTest::mouseClick(coronalWindow, Qt::LeftButton, Qt::NoModifier, newPoint);

  newPosition = d->Viewer->GetSelectedPosition();
  QVERIFY(newPosition[0] != lastPosition[0]);
  QCOMPARE(newPosition[1], lastPosition[1]);
  QVERIFY(newPosition[2] != lastPosition[2]);
  QCOMPARE(d->StateTester->GetItkSignals(axialSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(sagittalSnc).size(), std::size_t(1));
  QCOMPARE(d->StateTester->GetItkSignals(coronalSnc).size(), std::size_t(0));

  d->StateTester->Clear();
}


// --------------------------------------------------------------------------
void niftkSingleViewerWidgetTestClass::testSelectRenderWindowByInteraction()
{
  Q_D(niftkSingleViewerWidgetTestClass);

  QmitkRenderWindow* coronalWindow = d->Viewer->GetCoronalWindow();
  QmitkRenderWindow* sagittalWindow = d->Viewer->GetSagittalWindow();

  QCOMPARE(d->Viewer->IsSelected(), true);
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), coronalWindow);

  d->Viewer->SetWindowLayout(WINDOW_LAYOUT_ORTHO);

  QCOMPARE(d->Viewer->IsSelected(), true);
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), coronalWindow);

  d->StateTester->Clear();

  QPoint centre = sagittalWindow->rect().center();
  QTest::mouseClick(sagittalWindow, Qt::LeftButton, Qt::NoModifier, centre);

  QCOMPARE(d->Viewer->IsSelected(), true);
  QCOMPARE(d->Viewer->GetSelectedRenderWindow(), sagittalWindow);
}


// --------------------------------------------------------------------------
static void ShiftArgs(int& argc, char* argv[], int steps = 1)
{
  /// We exploit that there must be a NULL pointer after the arguments.
  /// (Guaranteed by the standard.)
  int i = 1;
  do
  {
    argv[i] = argv[i + steps];
    ++i;
  }
  while (argv[i - 1]);
  argc -= steps;
}


// --------------------------------------------------------------------------
int niftkSingleViewerWidgetTest(int argc, char* argv[])
{
  QApplication app(argc, argv);
  Q_UNUSED(app);

  std::srand((unsigned) std::time(0));

  niftkSingleViewerWidgetTestClass test;

  std::string interactiveModeOption("-i");
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == interactiveModeOption)
    {
      test.SetInteractiveMode(true);
      ::ShiftArgs(argc, argv);
      break;
    }
  }

  if (argc < 2)
  {
    MITK_INFO << "Missing argument. No image file given.";
    return 1;
  }

  test.SetFileName(argv[1]);
  ::ShiftArgs(argc, argv);

  /// We used the arguments to initialise the test. No arguments is passed
  /// to the Qt test, so that all the test functions are executed.
//  argc = 1;
//  argv[1] = NULL;
  return QTest::qExec(&test, argc, argv);
}
