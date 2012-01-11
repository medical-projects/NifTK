/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-12-16 09:12:58 +0000 (Fri, 16 Dec 2011) $
 Revision          : $Revision: 8039 $
 Last modified by  : $Author: mjc $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include "QmitkMIDASMultiViewVisibilityManager.h"
#include "QmitkMIDASRenderWindow.h"
#include "QmitkMIDASSingleViewWidget.h"
#include "mitkBaseRenderer.h"
#include "mitkVtkResliceInterpolationProperty.h"
#include "mitkDataStorageUtils.h"
#include "vtkRenderWindow.h"

QmitkMIDASMultiViewVisibilityManager::QmitkMIDASMultiViewVisibilityManager(mitk::DataStorage::Pointer dataStorage)
: m_InDataStorageChanged(false)
{
  assert(dataStorage);
  m_DataStorage = dataStorage;

  m_DataStorage->AddNodeEvent.AddListener( mitk::MessageDelegate1<QmitkMIDASMultiViewVisibilityManager, const mitk::DataNode*>
    ( this, &QmitkMIDASMultiViewVisibilityManager::NodeAddedProxy ) );

  m_DataStorage->ChangedNodeEvent.AddListener( mitk::MessageDelegate1<QmitkMIDASMultiViewVisibilityManager, const mitk::DataNode*>
    ( this, &QmitkMIDASMultiViewVisibilityManager::NodeChangedProxy ) );

}

QmitkMIDASMultiViewVisibilityManager::~QmitkMIDASMultiViewVisibilityManager()
{
}

void QmitkMIDASMultiViewVisibilityManager::RegisterWidget(QmitkMIDASSingleViewWidget *widget)
{
  widget->SetDataStorage(m_DataStorage);

  std::vector<mitk::DataNode*> newNodes;
  m_ListOfDataNodes.push_back(newNodes);
  m_ListOfWidgets.push_back(widget);
}

void QmitkMIDASMultiViewVisibilityManager::NodeAddedProxy( const mitk::DataNode* node )
{
  // Guarantee no recursions when a new node event is thrown in NodeAdded()
  if(!m_InDataStorageChanged)
  {
    m_InDataStorageChanged = true;
    this->NodeAdded(node);
    m_InDataStorageChanged = false;
  }
}

void QmitkMIDASMultiViewVisibilityManager::NodeAdded( const mitk::DataNode* node)
{
  this->SetInitialNodeProperties(const_cast<mitk::DataNode*>(node));
}

void QmitkMIDASMultiViewVisibilityManager::SetInitialNodeProperties(mitk::DataNode* node)
{
  // So as each new node is added (i.e. surfaces, point sets, images) we set default visibility to false.
  for (unsigned int i = 0; i < m_ListOfWidgets.size(); i++)
  {
    QmitkMIDASRenderWindow* window = m_ListOfWidgets[i]->GetRenderWindow();
    mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(window->GetVtkRenderWindow());
    node->SetBoolProperty("visible", false, renderer);
  }

  // For MIDAS, which might have a light background in the render window, we need to make sure black is not transparent.
  if (dynamic_cast<mitk::Image*>(node->GetData()))
  {
    node->SetProperty("black opacity", mitk::FloatProperty::New(1));
  }

  if (m_DefaultInterpolation == MIDAS_INTERPOLATION_NONE)
  {
    node->SetProperty("texture interpolation", mitk::BoolProperty::New(false));
  }
  else
  {
    node->SetProperty("texture interpolation", mitk::BoolProperty::New(true));
  }

  if (m_DefaultInterpolation == MIDAS_INTERPOLATION_NONE)
  {
    node->SetProperty("reslice interpolation", mitk::VtkResliceInterpolationProperty::New("VTK_RESLICE_NEAREST"));
  }
  else if (m_DefaultInterpolation == MIDAS_INTERPOLATION_LINEAR)
  {
    node->SetProperty("reslice interpolation", mitk::VtkResliceInterpolationProperty::New("VTK_RESLICE_LINEAR"));
  }
  else if (m_DefaultInterpolation == MIDAS_INTERPOLATION_CUBIC)
  {
    node->SetProperty("reslice interpolation", mitk::VtkResliceInterpolationProperty::New("VTK_RESLICE_CUBIC"));
  }
}

void QmitkMIDASMultiViewVisibilityManager::NodeChangedProxy( const mitk::DataNode* node )
{
  // Guarantee no recursions when a new node event is thrown in NodeAdded()
  if(!m_InDataStorageChanged)
  {
    m_InDataStorageChanged = true;
    this->NodeChanged(node);
    m_InDataStorageChanged = false;
  }
}

void QmitkMIDASMultiViewVisibilityManager::NodeChanged( const mitk::DataNode* node)
{
  this->UpdateNodeProperties(const_cast<mitk::DataNode*>(node));
}

void QmitkMIDASMultiViewVisibilityManager::UpdateNodeProperties(mitk::DataNode* node)
{
  for (unsigned int i = 0; i < m_ListOfDataNodes.size(); i++)
  {
    for (unsigned int j = 0; j < m_ListOfDataNodes[i].size(); j++)
    {
      if ((m_ListOfDataNodes[i])[j] == node)
      {
        bool visibility(false);
        node->GetBoolProperty("visible", visibility);

        QmitkMIDASRenderWindow* window = m_ListOfWidgets[i]->GetRenderWindow();
        mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(window->GetVtkRenderWindow());
        node->SetBoolProperty("visible", visibility, renderer);
      }
    }
  }
}

unsigned int QmitkMIDASMultiViewVisibilityManager::GetIndexFromWindow(QmitkMIDASRenderWindow* window)
{
  int result = -1;

  for (unsigned int i = 0; i < m_ListOfWidgets.size(); i++)
  {
    bool contains = m_ListOfWidgets[i]->ContainsWindow(window);
    if (contains)
    {
      result = i;
      break;
    }
  }
  return result;
}

void QmitkMIDASMultiViewVisibilityManager::RemoveNodesFromWindow(int windowIndex)
{
  QmitkMIDASSingleViewWidget *widget = m_ListOfWidgets[windowIndex];
  assert(widget);

  QmitkMIDASRenderWindow* window = widget->GetRenderWindow();
  vtkRenderWindow *vtkRenderWindow = window->GetVtkRenderWindow();
  assert(vtkRenderWindow);

  mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(vtkRenderWindow);
  assert(renderer);

  for (unsigned int j = 0; j < (m_ListOfDataNodes[windowIndex]).size(); j++)
  {
    m_ListOfDataNodes[windowIndex][j]->SetProperty("visible", mitk::BoolProperty::New(false), renderer);
  }

  (m_ListOfDataNodes[windowIndex]).clear();
}

void QmitkMIDASMultiViewVisibilityManager::AddNodeToWindow(int windowIndex, mitk::DataNode* node)
{
  QmitkMIDASSingleViewWidget *widget = m_ListOfWidgets[windowIndex];
  assert(widget);

  QmitkMIDASRenderWindow* window = widget->GetRenderWindow();
  vtkRenderWindow *vtkRenderWindow = window->GetVtkRenderWindow();
  assert(vtkRenderWindow);

  mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(vtkRenderWindow);
  assert(renderer);

  node->SetProperty("visible", mitk::BoolProperty::New(true), renderer);

  (m_ListOfDataNodes[windowIndex]).push_back(node);
}

mitk::TimeSlicedGeometry::Pointer QmitkMIDASMultiViewVisibilityManager::GetGeometry(std::vector<mitk::DataNode*> nodes, unsigned int nodeIndex)
{
  mitk::TimeSlicedGeometry::Pointer geometry = NULL;
  int indexThatWeActuallyUsed = -1;

  // If nodeIndex < 0, we are choosing the best geometry from all available nodes.
  if (nodeIndex < 0)
  {

    // First try to find an image geometry, and if so, use the first one.
    mitk::Image::Pointer image = NULL;
    for (unsigned int i = 0; i < nodes.size(); i++)
    {
      image = dynamic_cast<mitk::Image*>(nodes[i]->GetData());
      if (image.IsNotNull())
      {
        geometry = image->GetTimeSlicedGeometry();
        indexThatWeActuallyUsed = i;
        break;
      }
    }

    // Failing that, use the first geometry available.
    if (geometry.IsNull())
    {
      for (unsigned int i = 0; i < nodes.size(); i++)
      {
        mitk::BaseData::Pointer data = nodes[i]->GetData();
        if (data.IsNotNull())
        {
          geometry = data->GetTimeSlicedGeometry();
          indexThatWeActuallyUsed = i;
          break;
        }
      }
    }
  }
  // So, the caller has nominated a specific node, lets just use that one.
  else if (nodeIndex >= 0 && nodeIndex < nodes.size())
  {
    mitk::BaseData::Pointer data = nodes[nodeIndex]->GetData();
    if (data.IsNotNull())
    {
      geometry = data->GetTimeSlicedGeometry();
      indexThatWeActuallyUsed = nodeIndex;
    }
  }
  // Essentially, the nodeIndex is garbage, so just pick the first one.
  else
  {
    mitk::BaseData::Pointer data = nodes[0]->GetData();
    if (data.IsNotNull())
    {
      geometry = data->GetTimeSlicedGeometry();
      indexThatWeActuallyUsed = 0;
    }
  }

  // In addition, (as MIDAS is an image based viewer), if the node is NOT a greyscale image,
  // we try and search the parents of the node to find a greyscale image and use that one in preference.
  // This assumes that derived datasets, such as point sets, surfaces, segmented volumes are correctly assigned to parents.
  if (indexThatWeActuallyUsed != -1)
  {
    if (!mitk::IsNodeAGreyScaleImage(nodes[indexThatWeActuallyUsed]))
    {
      mitk::DataNode::Pointer node = FindParentGreyScaleImage(this->m_DataStorage, nodes[indexThatWeActuallyUsed]);
      if (node.IsNotNull())
      {
        mitk::BaseData::Pointer data = nodes[0]->GetData();
        if (data.IsNotNull())
        {
          geometry = data->GetTimeSlicedGeometry();
        }
      }
    }
  }
  return geometry;
}

void QmitkMIDASMultiViewVisibilityManager::OnNodesDropped(QmitkMIDASRenderWindow *window, std::vector<mitk::DataNode*> nodes)
{

  // Works out the initial window index that the image is dropped into.
  // Remember:
  //   There are always 5x5 windows, arranged in row order.
  //   These may or may not be visible, so for example if you have 2x2 visible,
  //   then this corresponds to indexes 0,1 then skip 2,3,4, then 5,6 are visible.

  int windowIndex = this->GetIndexFromWindow(window);

  if (m_DataStorage.IsNotNull() && windowIndex != -1)
  {
    for (unsigned int i = 0; i < nodes.size(); i++)
    {
      std::string name;
      if (nodes[i] != 0 && nodes[i]->GetStringProperty("name", name))
      {
        MITK_INFO << "Dropped " << nodes.size() << " into window[" << windowIndex <<"], name[" << i << "]=" << name << std::endl;
      }
    }

    if (m_DropType == MIDAS_DROP_TYPE_SINGLE)
    {

      MITK_INFO << "Dropped single" << std::endl;

      mitk::TimeSlicedGeometry::Pointer geometry = this->GetGeometry(nodes, -1);
      if (geometry.IsNull())
      {
        MITK_ERROR << "Error, dropping " << nodes.size() << " nodes into window " << windowIndex << ", could not find geometry which must be a programming bug." << std::endl;
        return;
      }

      // Clear all nodes from the single window denoted by windowIndex (the one that was dropped into).
      this->RemoveNodesFromWindow(windowIndex);

      // Then add all nodes into the same window denoted by windowIndex (the one that was dropped into).
      for (unsigned int i = 0; i < nodes.size(); i++)
      {
        this->AddNodeToWindow(windowIndex, nodes[i]);
      }

      // Then set up geometry of that single window.
      m_ListOfWidgets[windowIndex]->InitializeGeometry(geometry.GetPointer());
      m_ListOfWidgets[windowIndex]->SetViewOrientation(QmitkMIDASSingleViewWidget::MIDAS_VIEW_SAGITTAL);

    }
    else if (m_DropType == MIDAS_DROP_TYPE_MULTIPLE)
    {
      MITK_INFO << "Dropped multiple" << std::endl;

      // Work out which window we are actually dropping into.
      // We aim to put one object, in each of consecutive windows.
      // If we hit the end (of the 5x5=25 list), we go back to zero.

      unsigned int dropIndex = windowIndex;

      for (unsigned int i = 0; i < nodes.size(); i++)
      {
        while (dropIndex < m_ListOfWidgets.size() && !m_ListOfWidgets[dropIndex]->isVisible())
        {
          // i.e. if the window we are in, is not visible, keep looking
          dropIndex++;
        }
        if (dropIndex == m_ListOfWidgets.size())
        {
          // give up? Or we could go back to zero?
          dropIndex = 0;
        }

        mitk::TimeSlicedGeometry::Pointer geometry = this->GetGeometry(nodes, i);
        if (geometry.IsNull())
        {
          MITK_ERROR << "Error, dropping node " << i << ", from a list of " << nodes.size() << " nodes into window " << dropIndex << ", could not find geometry which must be a programming bug." << std::endl;
          return;
        }

        // So we are removing all images that are present from the window denoted by dropIndex,
        this->RemoveNodesFromWindow(dropIndex);

        // ...and then adding a single image to that window, denoted by dropIndex.
        this->AddNodeToWindow(dropIndex, nodes[i]);

        // Initialise geometry according to first image
        m_ListOfWidgets[dropIndex]->InitializeGeometry(geometry.GetPointer());
        m_ListOfWidgets[dropIndex]->SetViewOrientation(QmitkMIDASSingleViewWidget::MIDAS_VIEW_SAGITTAL);

        // We need to always increment by at least one window, or else infinite loop-a-rama.
        dropIndex++;
      }
    }
    else if (m_DropType == MIDAS_DROP_TYPE_ALL)
    {
      MITK_INFO << "Dropped thumbnail" << std::endl;

      mitk::TimeSlicedGeometry::Pointer geometry = this->GetGeometry(nodes, -1);
      if (geometry.IsNull())
      {
        MITK_ERROR << "Error, dropping " << nodes.size() << " nodes into window " << windowIndex << ", could not find geometry which must be a programming bug." << std::endl;
        return;
      }

      // Sort out visibility properties and the like for every window.
      for (unsigned int i = 0; i < m_ListOfWidgets.size(); i++)
      {
        // Clear all nodes from every window.
        this->RemoveNodesFromWindow(i);

        // Then add all nodes into every window.
        for (unsigned int j = 0; j < nodes.size(); j++)
        {
          this->AddNodeToWindow(i, nodes[j]);
        }
      }

      // Then sort out geometry for every window, with an increasing slice number evenly throughout the volume.
      for (unsigned int i = 0; i < m_ListOfWidgets.size(); i++)
      {
        m_ListOfWidgets[i]->InitializeGeometry(geometry.GetPointer());
        m_ListOfWidgets[i]->SetViewOrientation(QmitkMIDASSingleViewWidget::MIDAS_VIEW_SAGITTAL);

        unsigned int minSlice = m_ListOfWidgets[i]->GetMinSlice();
        unsigned int maxSlice = m_ListOfWidgets[i]->GetMaxSlice();
        unsigned int numberOfEdgeSlicesToIgnore = (maxSlice - minSlice + 1) * 0.05; // ignore first and last 5 percent, as usually junk.
        unsigned int remainingNumberOfSlices = (maxSlice - minSlice + 1) - 2 * numberOfEdgeSlicesToIgnore;
        float fraction = (float)i/(float)(m_ListOfWidgets.size());
        unsigned int chosenSlice = numberOfEdgeSlicesToIgnore + remainingNumberOfSlices*fraction;

        MITK_INFO << "Dropping thumbnail, i=" << i \
            << ", minSlice=" << minSlice \
            << ", maxSlice=" << maxSlice \
            << ", numberOfEdgeSlicesToIgnore=" << numberOfEdgeSlicesToIgnore \
            << ", remainingNumberOfSlices=" << remainingNumberOfSlices \
            << ", fraction=" << fraction \
            << ", chosenSlice=" << chosenSlice << std::endl;
        m_ListOfWidgets[i]->SetSliceNumber(chosenSlice);

      }
    }
  }
}

