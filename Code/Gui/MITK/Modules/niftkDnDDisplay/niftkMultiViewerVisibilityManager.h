/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkMultiViewerVisibilityManager_h
#define niftkMultiViewerVisibilityManager_h

#include <niftkDnDDisplayExports.h>
#include <mitkMIDASEnums.h>
#include "niftkSingleViewerWidget.h"
#include <QWidget>
#include <mitkDataStorage.h>
#include <mitkBaseProperty.h>
#include <mitkProperties.h>
#include <itkImage.h>

class QmitkRenderWindow;
class niftkSingleViewerWidget;

/**
 * \class niftkMultiViewerVisibilityManager
 * \brief Maintains a list of niftkSingleViewerWidgets and coordinates visibility
 * properties by listening to AddNodeEvent, RemoveNodeEvent and listening directly
 * to Modified events from the nodes "visibility" property in DataStorage.
 */
class NIFTKDNDDISPLAY_EXPORT niftkMultiViewerVisibilityManager : public QObject
{
  Q_OBJECT

public:

  /// \brief This class must (checked with assert) have a non-NULL mitk::DataStorage so it is injected in the constructor, and we register to AddNodeEvent, RemoveNodeEvent.
  niftkMultiViewerVisibilityManager(mitk::DataStorage::Pointer dataStorage);

  /// \brief Destructor, which unregisters all the listeners.
  virtual ~niftkMultiViewerVisibilityManager();

  /// \brief Each new viewer should first be registered with this class, so this class can manage renderer specific visibility properties.
  void RegisterViewer(niftkSingleViewerWidget *viewer);

  /// \brief De-registers a range of viewers, which means actually removing them from m_DataNodes and m_Viewers.
  /// Start index inclusive, end index exclusive.
  void DeregisterViewers(std::size_t startIndex = 0, std::size_t endIndex = -1);

  /// \brief Used to clear a range of windows, meaning to set renderer specific visibility properties to false for all the nodes registered in m_DataNodes.
  /// Start index inclusive, end index exclusive.
  void ClearViewers(std::size_t startIndex = 0, std::size_t endIndex = -1);

  /// \brief Called when a DataStorage AddNodeEvent was emmitted and calls NodeAdded afterwards.
  void NodeAddedProxy(const mitk::DataNode* node);

  /// \brief Called when a DataStorage RemoveNodeEvent was emmitted and calls NodeRemoved afterwards.
  void NodeRemovedProxy(const mitk::DataNode* node);

  /// \brief Called when the visibility property changes in DataStorage, and we update renderer specific visibility properties accordingly.
  void OnGlobalVisibilityChanged(mitk::DataNode* node);

  /// \brief Set the drop type, which controls the behaviour when multiple images are dropped into a single viewer.
  void SetDropType(DnDDisplayDropType dropType) { m_DropType = dropType; }

  /// \brief Get the drop type, which controls the behaviour when multiple images are dropped into a single viewer.
  DnDDisplayDropType GetDropType() const { return m_DropType; }

  /// \brief Sets the default interpolation type, which takes effect when a new image is dropped.
  void SetInterpolationType(DnDDisplayInterpolationType interpolationType) { m_InterpolationType = interpolationType; }

  /// \brief Returns the default interpolation type, which takes effect when a new image is dropped.
  DnDDisplayInterpolationType GetInterpolationType() const { return m_InterpolationType; }

  /// \brief Sets the default render window layout for when images are dropped into a render window.
  void SetDefaultWindowLayout(WindowLayout windowLayout) { m_DefaultWindowLayout = windowLayout; }

  /// \brief Returns the default render window layout for when images are dropped into a render window.
  WindowLayout GetDefaultWindowLayout() const { return m_DefaultWindowLayout; }

  /// \brief When we drop nodes onto a window, if true, we add all the children.
  void SetAutomaticallyAddChildren(bool autoAdd) { m_AutomaticallyAddChildren = autoAdd; }

  /// \brief When we drop nodes onto a window, if true, we add all the children.
  bool GetAutomaticallyAddChildren() const { return m_AutomaticallyAddChildren; }

  /// \brief Sets the flag deciding whether we prefer to accumulate images each time they are dropped.
  void SetAccumulateWhenDropping(bool accumulate) { m_Accumulate = accumulate; }

  /// \brief Gets the flag deciding whether we accumulate images each time we drop.
  bool GetAccumulateWhenDropped() const { return m_Accumulate; }

  /// \brief Gets the number of nodes currently visible in a window.
  virtual int GetNodesInViewer(int windowIndex);

public slots:

  /// \brief When nodes are dropped, we set all the default properties, and renderer specific visibility flags etc.
  void OnNodesDropped(niftkSingleViewerWidget* viewer, std::vector<mitk::DataNode*> nodes);

signals:

protected:

  /// \brief Called when a node is added, and we set rendering window specific visibility
  /// to false for all registered windows, plus other default properties such as interpolation type.
  virtual void NodeAdded(const mitk::DataNode* node);

  /// \brief Called when a node is added, and we set rendering window specific visibility
  virtual void NodeRemoved(const mitk::DataNode* node);

  /// \brief For a given window, effectively sets the rendering window specific visibility property for the given node to initialVisibility.
  virtual void AddNodeToViewer(int windowIndex, mitk::DataNode* node, bool visibility=true);

  /// \brief For a given window (denoted by its windowIndex, or index number in m_Viewers), effectively sets the rendering window specific visibility property of all nodes registered with that window to false.
  virtual void RemoveNodesFromViewer(int windowIndex);

protected slots:

private:

  /// \brief Works out the correct window layout from the data, and from the preferences.
  WindowLayout GetWindowLayout(std::vector<mitk::DataNode*> nodes);

  /// \brief ITK templated method (accessed via MITK access macros) to work out the orientation in the XY plane.
  template<typename TPixel, unsigned int VImageDimension>
  void GetAsAcquiredOrientation(
      itk::Image<TPixel, VImageDimension>* itkImage,
      MIDASOrientation &outputOrientation
      );

  // Will retrieve the correct geometry from a list of nodes.
  // If nodeIndex < 0 (for single drop case).
  //   Search for first available image
  //   Failing that, first geometry.
  // If node index >=0, and < nodes.size()
  //   Picks out the geometry of the object for that index.
  // Else
  //   Picks out the first geometry.
  mitk::TimeGeometry::Pointer GetTimeGeometry(std::vector<mitk::DataNode*> nodes, int nodeIndex);

  // This object MUST be connected to a datastorage, hence it is passed in via the constructor.
  mitk::DataStorage::Pointer m_DataStorage;

  // We maintain a set of data nodes present in each window.
  // So, it's a vector, as we have one set for each of the registered windows.
  std::vector< std::set<mitk::DataNode*> > m_DataNodesPerViewer;

  // Additionally, we manage a list of viewers, where m_DataNodes.size() == m_Viewers.size() should always be true.
  std::vector< niftkSingleViewerWidget* > m_Viewers;

  // We also observe all the global visibility properties for each registered node.
  std::map<mitk::BaseProperty*, unsigned long> m_GlobalVisibilityObserverTags;

  // Simply keeps track of whether we are currently processing an update to avoid repeated/recursive calls.
  bool m_BlockDataStorageEvents;

  // Keeps track of the current mode, as it effects the response when images are dropped, as images are spread over single, multiple or all windows.
  DnDDisplayDropType m_DropType;

  // Keeps track of the default window layout, as it affects the response when images are dropped, as the image should be oriented axial, coronal, sagittal, or as acquired (as per the X-Y plane).
  WindowLayout m_DefaultWindowLayout;

  // Keeps track of the default interpolation, as it affects the response when images are dropped,
  // as the dropped image should switch to that interpolation type, although as it is a node based property will affect all windows.
  DnDDisplayInterpolationType m_InterpolationType;

  // Boolean to indicate whether to automatically add children, default to true.
  bool m_AutomaticallyAddChildren;

  // Boolean to indicate whether successive drops into the same window are cumulative.
  bool m_Accumulate;

};

#endif