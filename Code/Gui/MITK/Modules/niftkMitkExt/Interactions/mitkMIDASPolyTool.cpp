/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-11-19 22:31:43 +0000 (Sat, 19 Nov 2011) $
 Revision          : $Revision: 7815 $
 Last modified by  : $Author: mjc $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include "mitkMIDASPolyTool.h"
#include "mitkMIDASPolyToolEventInterface.h"
#include "mitkMIDASPolyToolOpAddToFeedbackContour.h"
#include "mitkMIDASPolyToolOpUpdateFeedbackContour.h"
#include "mitkMIDASPolyTool.xpm"
#include "mitkOperationEvent.h"
#include "mitkUndoController.h"
#include "mitkPointUtils.h"
#include "mitkToolManager.h"
#include "mitkBaseRenderer.h"
#include "mitkContour.h"

const std::string mitk::MIDASPolyTool::MIDAS_POLY_TOOL_ANCHOR_POINTS("MIDAS PolyTool anchor points");
const std::string mitk::MIDASPolyTool::MIDAS_POLY_TOOL_PREVIOUS_CONTOUR("MIDAS PolyTool previous contour");
const mitk::OperationType mitk::MIDASPolyTool::MIDAS_POLY_TOOL_OP_ADD_TO_FEEDBACK_CONTOUR = 320420;
const mitk::OperationType mitk::MIDASPolyTool::MIDAS_POLY_TOOL_OP_UPDATE_FEEDBACK_CONTOUR = 320421;

namespace mitk{
  MITK_TOOL_MACRO(NIFTKMITKEXT_EXPORT, MIDASPolyTool, "MIDAS Poly Tool");
}

mitk::MIDASPolyTool::MIDASPolyTool() : MIDASContourTool("MIDASPolyTool")
, m_ReferencePoints(NULL)
, m_PreviousContourReferencePoints(NULL)
, m_PreviousContour(NULL)
, m_PreviousContourNode(NULL)
, m_PreviousContourVisible(false)
, m_PolyLinePointSet(NULL)
, m_PolyLinePointSetNode(NULL)
, m_PolyLinePointSetVisible(false)
{
  // great magic numbers, connecting interactor straight to method calls.
  CONNECT_ACTION( 12, OnLeftMousePressed );
  CONNECT_ACTION( 66, OnMiddleMousePressed );
  CONNECT_ACTION( 90, OnMiddleMousePressedAndMoved );
  CONNECT_ACTION( 76, OnMiddleMouseReleased );

  // These are not added to DataStorage as they are never drawn, they are just an internal data structure
  m_ReferencePoints = Contour::New();
  m_PreviousContourReferencePoints = Contour::New();

  // This point set is so we can highlight the first point (or potentially all points).
  m_PolyLinePointSet = mitk::PointSet::New();
  m_PolyLinePointSetNode = mitk::DataNode::New();
  m_PolyLinePointSetNode->SetData( m_PolyLinePointSet );
  m_PolyLinePointSetNode->SetProperty("name", mitk::StringProperty::New(MIDAS_POLY_TOOL_ANCHOR_POINTS) );
  m_PolyLinePointSetNode->SetProperty("visible", BoolProperty::New(true));
  m_PolyLinePointSetNode->SetProperty("helper object", BoolProperty::New(true));
  m_PolyLinePointSetNode->SetProperty("color", ColorProperty::New(1, 1, 0));

  // This is so we can draw the previous contour, as we stretch, and minipulate the contour with middle mouse button.
  m_PreviousContour = Contour::New();
  m_PreviousContourNode = DataNode::New();
  m_PreviousContourNode->SetData( m_PreviousContour );
  m_PreviousContourNode->SetProperty("name", StringProperty::New(MIDAS_POLY_TOOL_PREVIOUS_CONTOUR));
  m_PreviousContourNode->SetProperty("visible", BoolProperty::New(true));
  m_PreviousContourNode->SetProperty("helper object", BoolProperty::New(true));
  m_PreviousContourNode->SetProperty("Width", FloatProperty::New(1));
  m_PreviousContourNode->SetProperty("color", ColorProperty::New(0, 1, 0));

  this->Disable3dRenderingOfPreviousContour();

  m_Interface = new MIDASPolyToolEventInterface();
  m_Interface->SetMIDASPolyTool(this);
}

mitk::MIDASPolyTool::~MIDASPolyTool()
{
  if (m_Interface != NULL)
  {
    delete m_Interface;
  }
}

const char* mitk::MIDASPolyTool::GetName() const
{
  return "Poly";
}

const char** mitk::MIDASPolyTool::GetXPM() const
{
  return mitkMIDASPolyTool_xpm;
}

void mitk::MIDASPolyTool::Disable3dRenderingOfPreviousContour()
{
  this->Disable3dRenderingOfNode(m_PreviousContourNode);
}

void mitk::MIDASPolyTool::ClearData()
{
  mitk::MIDASContourTool::ClearData();

  // These are added to the DataManager, but only drawn when the middle mouse is down (and subsequently moved).
  m_PreviousContour->Initialize();
  m_PreviousContour->SetClosed(m_ContourClosed);
  m_PreviousContour->SetWidth(m_ContourWidth);

  // These are not added to the DataManager, so will never be drawn.
  m_ReferencePoints->Initialize();
  m_ReferencePoints->SetClosed(m_ContourClosed);
  m_ReferencePoints->SetWidth(m_ContourWidth);

  // These are not added to the DataManager, so will never be drawn.
  m_PreviousContourReferencePoints->Initialize();
  m_PreviousContourReferencePoints->SetClosed(m_ContourClosed);
  m_PreviousContourReferencePoints->SetWidth(m_ContourWidth);
}

void mitk::MIDASPolyTool::Activated()
{
  MIDASTool::Activated();

  DataNode* workingNode( m_ToolManager->GetWorkingData(0) );
  if (!workingNode) return;

  // Store these for later (in base class), as dynamic casts are slow. HOWEVER, IT IS NOT THREAD SAFE.
  m_WorkingImage = dynamic_cast<Image*>(workingNode->GetData());
  m_WorkingImageGeometry = m_WorkingImage->GetGeometry();

  // If these are not set, something is fundamentally wrong.
  assert(m_WorkingImage);
  assert(m_WorkingImageGeometry);

  // Initialize data, and set properties.
  this->ClearData();

  // Turn the contours on, and set to correct colour. The FeedBack contour is yellow.
  FeedbackContourTool::SetFeedbackContourVisible(true);
  FeedbackContourTool::SetFeedbackContourColor(1, 1, 0);

  MIDASContourTool::SetBackgroundContourVisible(false);
  MIDASContourTool::SetBackgroundContourColorDefault();

  // Just to be explicit
  this->Disable3dRenderingOfPreviousContour();
  this->SetPreviousContourVisible(false);
  this->SetPolyLinePointSetVisible(false);
}

void mitk::MIDASPolyTool::Deactivated()
{
  MIDASTool::Deactivated();

  Contour* feedbackContour = FeedbackContourTool::GetFeedbackContour();
  this->AccumulateContourInWorkingData(*feedbackContour, 2);

  // Initialize data, and set properties.
  this->ClearData();

  // Set visibility.
  FeedbackContourTool::SetFeedbackContourVisible(false);
  MIDASContourTool::SetBackgroundContourVisible(false);
  this->SetPreviousContourVisible(false);
  this->SetPolyLinePointSetVisible(false);
  this->RenderAllWindows();
}

void mitk::MIDASPolyTool::SetPreviousContourVisible(bool visible)
{
  this->Disable3dRenderingOfPreviousContour();

  if (m_PreviousContourVisible == visible)
  {
    return;
  }
  if ( DataStorage* storage = m_ToolManager->GetDataStorage() )
  {
    if (visible)
    {
      storage->Add( m_PreviousContourNode );
    }
    else
    {
      storage->Remove( m_PreviousContourNode );
    }
  }
  m_PreviousContourVisible = visible;
}

void mitk::MIDASPolyTool::SetPolyLinePointSetVisible(bool visible)
{
  if (m_PolyLinePointSetVisible == visible)
  {
    return;
  }
  if ( DataStorage* storage = m_ToolManager->GetDataStorage() )
  {
    if (visible)
    {
      storage->Add( m_PolyLinePointSetNode );
    }
    else
    {
      storage->Remove( m_PolyLinePointSetNode );
    }
  }
  m_PolyLinePointSetVisible = visible;
}

void mitk::MIDASPolyTool::DrawWholeContour(
    const mitk::Contour& contourReferencePointsInput,
    const PlaneGeometry& planeGeometry,
    mitk::Contour& feedbackContour,
    mitk::Contour& backgroundContour
    )
{
  // If these are not set, something is fundamentally wrong.
  assert(m_WorkingImage);
  assert(m_WorkingImageGeometry);

  // Get the vector of points.
  mitk::Contour::PointsContainerPointer points = contourReferencePointsInput.GetPoints();

  // Reset the contours, as we are redrawing the whole thing.
  feedbackContour.Initialize();
  backgroundContour.Initialize();

  // Only bother drawing if we have at least two points
  if (points->Size() > 1)
  {
    mitk::Point3D p1;
    mitk::Point3D p2;

    p1 = points->ElementAt(0);

    for (unsigned long i = 1; i < points->Size(); i++)
    {
      p2 = points->ElementAt(i);

      this->DrawLineAroundVoxelEdges
        (
            *m_WorkingImage,
            *m_WorkingImageGeometry,
            planeGeometry,
            p2,
            p1,
            feedbackContour,
            backgroundContour
        );

      p1 = p2;

    }
  }
}

void mitk::MIDASPolyTool::UpdateFeedbackContour(
    const mitk::Point3D& closestCornerPoint,
    const PlaneGeometry& planeGeometry,
    mitk::Contour& contourReferencePointsInput,
    mitk::Contour& feedbackContour,
    mitk::Contour& backgroundContour,
    bool provideUndo
    )
{
  // Find closest point in reference points
  float distance = std::numeric_limits<float>::max();
  float closestDistance = std::numeric_limits<float>::max();
  unsigned int closestPointIndex = 0;
  mitk::Point3D closestPoint;
  mitk::Point3D p1;
  mitk::Point3D p2;
  mitk::Contour::PointsContainerPointer points = contourReferencePointsInput.GetPoints();

  for (unsigned long i = 0; i < contourReferencePointsInput.GetNumberOfPoints(); i++)
  {
    p1 = points->ElementAt(i);
    distance = mitk::GetSquaredDistanceBetweenPoints(p1, closestCornerPoint);
    if (distance < closestDistance)
    {
      closestDistance = distance;
      closestPointIndex = i;
      closestPoint = p1;
    }
  }

  if (provideUndo)
  {
    mitk::MIDASPolyToolOpUpdateFeedbackContour *doOp = new mitk::MIDASPolyToolOpUpdateFeedbackContour(
        MIDAS_POLY_TOOL_OP_UPDATE_FEEDBACK_CONTOUR,
        closestPointIndex,
        closestCornerPoint,
        &feedbackContour,
        &planeGeometry
        );


    mitk::MIDASPolyToolOpUpdateFeedbackContour *undoOp = new mitk::MIDASPolyToolOpUpdateFeedbackContour(
        MIDAS_POLY_TOOL_OP_UPDATE_FEEDBACK_CONTOUR,
        closestPointIndex,
        m_PreviousContour->GetPoints()->GetElement(closestPointIndex),
        &feedbackContour,
        &planeGeometry
        );

    mitk::OperationEvent* operationEvent = new mitk::OperationEvent( m_Interface, doOp, undoOp, "Update PolyLine");
    mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent( operationEvent );
    ExecuteOperation(doOp);
  }
  else
  {
    points->SetElement(closestPointIndex, closestCornerPoint);
    this->DrawWholeContour(contourReferencePointsInput, planeGeometry, feedbackContour, backgroundContour);
  }
}

void mitk::MIDASPolyTool::UpdateContours(Action* action, const StateEvent* stateEvent, bool provideUndo)
{
  if (m_ReferencePoints->GetNumberOfPoints() > 1)
  {
    // Don't forget to call baseclass method.
    MIDASContourTool::OnMousePressed(action, stateEvent);

    // If these are not set, something is fundamentally wrong.
    assert(m_WorkingImage);
    assert(m_WorkingImageGeometry);

    // Make sure we have valid contours, otherwise no point continuing.
    Contour* feedbackContour = FeedbackContourTool::GetFeedbackContour();
    assert(feedbackContour);

    Contour* backgroundContour = MIDASContourTool::GetBackgroundContour();
    assert(backgroundContour);

    // Make sure we have a valid position event, otherwise no point continuing.
    const PositionEvent* positionEvent = dynamic_cast<const PositionEvent*>(stateEvent->GetEvent());
    if (!positionEvent) return;

    // Make sure we have a valid geometry, otherwise no point continuing.
    const PlaneGeometry* planeGeometry( dynamic_cast<const PlaneGeometry*> (positionEvent->GetSender()->GetCurrentWorldGeometry2D() ) );
    if (!planeGeometry) return;

    // Convert mouse click to closest corner point, as in effect, we always draw from corner to corner.
    mitk::Point3D closestCornerPoint;
    this->ConvertPointToNearestVoxelCentreInMillimetreCoordinates(positionEvent->GetWorldPosition(), closestCornerPoint);

    // Redraw the "previous" contour line in green.
    this->DrawWholeContour(*(m_PreviousContourReferencePoints.GetPointer()), *planeGeometry, *(m_PreviousContour.GetPointer()), *backgroundContour);

    // Redraw the "current" contour line in yellow.
    this->UpdateFeedbackContour(closestCornerPoint, *planeGeometry, *(m_ReferencePoints.GetPointer()), *feedbackContour, *backgroundContour, provideUndo);

    // Make sure all views everywhere get updated.
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  }
}

/**
 * Poly lines are created by responding only to left mouse down.
 * When the tool is activated, the next mouse click starts the line.
 * We then keep adding points and lines until the tool is deactivated.
 */
bool mitk::MIDASPolyTool::OnLeftMousePressed (Action* action, const StateEvent* stateEvent)
{
  // Don't forget to call baseclass method.
  MIDASContourTool::OnMousePressed(action, stateEvent);

  // If these are not set, something is fundamentally wrong.
  assert(m_WorkingImage);
  assert(m_WorkingImageGeometry);

  // Make sure we have a valid position event, otherwise no point continuing.
  const PositionEvent* positionEvent = dynamic_cast<const PositionEvent*>(stateEvent->GetEvent());
  if (!positionEvent) return false;

  // Similarly, we can't do plane calculations if no geometry set.
  const PlaneGeometry* planeGeometry( dynamic_cast<const PlaneGeometry*> (positionEvent->GetSender()->GetCurrentWorldGeometry2D() ) );
  if (!planeGeometry) return false;

  // Convert mouse click to closest corner point, as in effect, we always draw from corner to corner.
  mitk::Point3D closestCornerPoint;
  this->ConvertPointToNearestVoxelCentreInMillimetreCoordinates(positionEvent->GetWorldPosition(), closestCornerPoint);

  mitk::Point3D previousPoint;
  if (m_ReferencePoints->GetNumberOfPoints() > 0)
  {
    previousPoint = m_ReferencePoints->GetPoints()->GetElement(m_ReferencePoints->GetNumberOfPoints() -1);
  }

  mitk::Contour::Pointer currentPoints = mitk::Contour::New();
  this->CopyContour(*(m_ReferencePoints.GetPointer()), *(currentPoints.GetPointer()));

  mitk::Contour::Pointer nextPoints = mitk::Contour::New();
  this->CopyContour(*(m_ReferencePoints.GetPointer()), *(nextPoints.GetPointer()));
  nextPoints->AddVertex(closestCornerPoint);

  mitk::MIDASPolyToolOpAddToFeedbackContour *doOp = new mitk::MIDASPolyToolOpAddToFeedbackContour(
      MIDAS_POLY_TOOL_OP_ADD_TO_FEEDBACK_CONTOUR,
      closestCornerPoint,
      nextPoints,
      planeGeometry
      );


  mitk::MIDASPolyToolOpAddToFeedbackContour *undoOp = new mitk::MIDASPolyToolOpAddToFeedbackContour(
      MIDAS_POLY_TOOL_OP_ADD_TO_FEEDBACK_CONTOUR,
      previousPoint,
      currentPoints,
      planeGeometry
      );

  mitk::OperationEvent* operationEvent = new mitk::OperationEvent( m_Interface, doOp, undoOp, "Add to PolyLine");
  mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent( operationEvent );
  ExecuteOperation(doOp);

  // Set this flag to indicate that we have stopped editing, which will trigger an update of the region growing.
  this->UpdateWorkingDataNodeBooleanProperty(0, mitk::MIDASContourTool::EDITING_PROPERTY_NAME, false);
  return true;
}

bool mitk::MIDASPolyTool::OnMiddleMousePressed (Action* action, const StateEvent* stateEvent)
{
  const PositionEvent* positionEvent = dynamic_cast<const PositionEvent*>(stateEvent->GetEvent());
  if (!positionEvent) return false;

  this->CopyContour(*(m_ReferencePoints), *(m_PreviousContourReferencePoints));
  this->SetPreviousContourVisible(true);

  this->UpdateContours(action, stateEvent, false);
  this->UpdateWorkingDataNodeBooleanProperty(0, mitk::MIDASContourTool::EDITING_PROPERTY_NAME, true);
  return true;
}

bool mitk::MIDASPolyTool::OnMiddleMousePressedAndMoved(Action* action, const StateEvent* stateEvent)
{
  const PositionEvent* positionEvent = dynamic_cast<const PositionEvent*>(stateEvent->GetEvent());
  if (!positionEvent) return false;

  this->UpdateContours(action, stateEvent, false);
  this->UpdateWorkingDataNodeBooleanProperty(0, mitk::MIDASContourTool::EDITING_PROPERTY_NAME, true);
  return true;
}

bool mitk::MIDASPolyTool::OnMiddleMouseReleased(Action* action, const StateEvent* stateEvent)
{
  this->SetPreviousContourVisible(false);

  this->UpdateContours(action, stateEvent, true);
  this->UpdateWorkingDataNodeBooleanProperty(0, mitk::MIDASContourTool::EDITING_PROPERTY_NAME, false);
  return true;
}

void mitk::MIDASPolyTool::ExecuteOperation(Operation* operation)
{
  if (!operation) return;

  mitk::MIDASContourTool::ExecuteOperation(operation);

  mitk::Contour* feedbackContour = FeedbackContourTool::GetFeedbackContour();
  assert(feedbackContour);

  // Retrieve the background contour, used to plot the closest straight line.
  mitk::Contour* backgroundContour = MIDASContourTool::GetBackgroundContour();
  assert(backgroundContour);

  switch (operation->GetOperationType())
  {
  case MIDAS_POLY_TOOL_OP_ADD_TO_FEEDBACK_CONTOUR:
    {
      MIDASPolyToolOpAddToFeedbackContour *op = static_cast<MIDASPolyToolOpAddToFeedbackContour*>(operation);
      if (op != NULL)
      {
        mitk::Point3D point = op->GetPoint();
        mitk::Contour* contour = op->GetContour();
        const mitk::PlaneGeometry* planeGeometry = op->GetPlaneGeometry();

        if (contour->GetNumberOfPoints() == 1)
        {
          m_PolyLinePointSet->InsertPoint(0, point);
          this->SetPolyLinePointSetVisible(true);
        }
        else
        {
          this->SetPolyLinePointSetVisible(false);
        }
        this->CopyContour(*contour, *(m_ReferencePoints.GetPointer()));
        this->m_MostRecentPointInMillimetres = point;
        this->DrawWholeContour(*(m_ReferencePoints.GetPointer()), *planeGeometry, *feedbackContour, *backgroundContour);
      }
    }
    break;
  case MIDAS_POLY_TOOL_OP_UPDATE_FEEDBACK_CONTOUR:
    {
      MIDASPolyToolOpUpdateFeedbackContour *op = static_cast<MIDASPolyToolOpUpdateFeedbackContour*>(operation);
      if (op != NULL)
      {
        unsigned int pointId = op->GetPointId();
        mitk::Point3D point = op->GetPoint();
        mitk::Contour* contour = op->GetContour();
        const mitk::PlaneGeometry* planeGeometry = op->GetPlaneGeometry();

        if (pointId >= 0 && pointId < contour->GetNumberOfPoints())
        {
          contour->GetPoints()->SetElement(pointId, point);
          this->DrawWholeContour(*contour, *planeGeometry, *feedbackContour, *backgroundContour);
        }
        else
        {
          MITK_ERROR << "Received invalid pointId=" << pointId << std::endl;
        }
      }
    }
    break;
  default:
    ;
  }

  // Signal that something has happened, and that it may be worth updating.
  ContoursHaveChanged.Send();

  // Make sure all views everywhere get updated.
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}
