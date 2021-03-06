/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkDataNodePropertyListener.h"

#include <itkCommand.h>

#include <mitkBaseRenderer.h>

namespace niftk
{

class PropertyChangedCommand : public itk::Command
{
public:
  mitkClassMacroItkParent(PropertyChangedCommand, itk::Command)
  mitkNewMacro3Param(PropertyChangedCommand, DataNodePropertyListener*, mitk::DataNode*, const mitk::BaseRenderer*)

  PropertyChangedCommand(DataNodePropertyListener* observer, mitk::DataNode* node, const mitk::BaseRenderer* renderer)
  : m_Observer(observer)
  , m_Node(node)
  , m_Renderer(renderer)
  {
    assert(observer);
    assert(node);
  }

  virtual ~PropertyChangedCommand()
  {
  }

  virtual void Execute(itk::Object* /*caller*/, const itk::EventObject& /*event*/) override
  {
    this->Notify();
  }

  virtual void Execute(const itk::Object* /*caller*/, const itk::EventObject& /*event*/) override
  {
    this->Notify();
  }

  void Notify()
  {
   if (!m_Observer->IsBlocked())
   {
      m_Observer->OnPropertyChanged(m_Node, m_Renderer);
   }
  }

private:
  DataNodePropertyListener* m_Observer;
  mitk::DataNode* m_Node;
  const mitk::BaseRenderer* m_Renderer;
};


//-----------------------------------------------------------------------------
DataNodePropertyListener::DataNodePropertyListener(const mitk::DataStorage::Pointer dataStorage, const std::string& propertyName)
: DataStorageListener(dataStorage)
, m_PropertyName(propertyName)
, m_DefaultValueType(NoType)
{
  assert(dataStorage.IsNotNull());

  this->AddAllObservers();
}


//-----------------------------------------------------------------------------
DataNodePropertyListener::DataNodePropertyListener(const mitk::DataStorage::Pointer dataStorage, const std::string& propertyName, bool defaultValue)
: DataStorageListener(dataStorage)
, m_PropertyName(propertyName)
, m_DefaultValueType(BoolType)
, m_BoolDefaultValue(defaultValue)
{
  assert(dataStorage.IsNotNull());

  this->AddAllObservers();
}


//-----------------------------------------------------------------------------
DataNodePropertyListener::DataNodePropertyListener(const mitk::DataStorage::Pointer dataStorage, const std::string& propertyName, int defaultValue)
: DataStorageListener(dataStorage)
, m_PropertyName(propertyName)
, m_DefaultValueType(IntType)
, m_IntDefaultValue(defaultValue)
{
  assert(dataStorage.IsNotNull());

  this->AddAllObservers();
}


//-----------------------------------------------------------------------------
DataNodePropertyListener::DataNodePropertyListener(const mitk::DataStorage::Pointer dataStorage, const std::string& propertyName, float defaultValue)
: DataStorageListener(dataStorage)
, m_PropertyName(propertyName)
, m_DefaultValueType(FloatType)
, m_FloatDefaultValue(defaultValue)
{
  assert(dataStorage.IsNotNull());

  this->AddAllObservers();
}


//-----------------------------------------------------------------------------
DataNodePropertyListener::DataNodePropertyListener(const mitk::DataStorage::Pointer dataStorage, const std::string& propertyName, const std::string& defaultValue)
: DataStorageListener(dataStorage)
, m_PropertyName(propertyName)
, m_DefaultValueType(StringType)
, m_StringDefaultValue(defaultValue)
{
  assert(dataStorage.IsNotNull());

  this->AddAllObservers();
}


//-----------------------------------------------------------------------------
DataNodePropertyListener::~DataNodePropertyListener()
{
  this->RemoveAllObservers();
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::SetRenderers(const std::vector<const mitk::BaseRenderer*>& renderers)
{
  if (renderers != m_Renderers)
  {
    this->RemoveAllObservers();

    m_Renderers = renderers;

    this->AddAllObservers();
  }
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::OnNodeAdded(mitk::DataNode* node)
{
  mitk::BaseProperty* globalProperty = node->GetProperty(m_PropertyName.c_str());
  if (!globalProperty)
  {
    if (m_DefaultValueType == BoolType)
    {
      node->SetBoolProperty(m_PropertyName.c_str(), m_BoolDefaultValue);
    }
    else if (m_DefaultValueType == IntType)
    {
      node->SetIntProperty(m_PropertyName.c_str(), m_IntDefaultValue);
    }
    else if (m_DefaultValueType == FloatType)
    {
      node->SetFloatProperty(m_PropertyName.c_str(), m_FloatDefaultValue);
    }
    else if (m_DefaultValueType == StringType)
    {
      node->SetStringProperty(m_PropertyName.c_str(), m_StringDefaultValue.c_str());
    }
  }

  for (std::size_t i = 0; i < m_Renderers.size(); ++i)
  {
    /// Note:
    /// GetProperty() returns the global property if there is no renderer specific property.
    /// Therefore, we need to check if the property is really renderer specific.
    mitk::BaseProperty* rendererSpecificProperty = node->GetProperty(m_PropertyName.c_str(), m_Renderers[i]);
    if (!rendererSpecificProperty)
    {
      if (m_DefaultValueType == BoolType)
      {
        node->SetBoolProperty(m_PropertyName.c_str(), m_BoolDefaultValue, const_cast<mitk::BaseRenderer*>(m_Renderers[i]));
      }
      else if (m_DefaultValueType == IntType)
      {
        node->SetIntProperty(m_PropertyName.c_str(), m_IntDefaultValue, const_cast<mitk::BaseRenderer*>(m_Renderers[i]));
      }
      else if (m_DefaultValueType == FloatType)
      {
        node->SetFloatProperty(m_PropertyName.c_str(), m_FloatDefaultValue, const_cast<mitk::BaseRenderer*>(m_Renderers[i]));
      }
      else if (m_DefaultValueType == StringType)
      {
        node->SetStringProperty(m_PropertyName.c_str(), m_StringDefaultValue.c_str(), const_cast<mitk::BaseRenderer*>(m_Renderers[i]));
      }
    }
  }

  Superclass::OnNodeAdded(node);
  this->AddObservers(node);
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::OnNodeRemoved(mitk::DataNode* node)
{
  Superclass::OnNodeRemoved(node);
  this->RemoveObservers(node);
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::OnNodeDeleted(mitk::DataNode* node)
{
  Superclass::OnNodeDeleted(node);
  this->RemoveObservers(node);
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::AddObservers(mitk::DataNode* node)
{
  if (!node)
  {
    return;
  }

  /// Note:
  /// We do *not* register the property observers to itk::AnyEvent that includes
  /// itk::ModifiedEvent and itk::DeleteEvent as well. We only register them to
  /// itk::ModifiedEvent.
  /// Registering them to both caused crash when a node was deleted, because the
  /// at node deletion the properties are deleted as well, and the OnPropertyChange
  /// handler was called while its 'host' node was being destructed.

  std::vector<unsigned long> propertyObserverTags(m_Renderers.size() + 1);

  mitk::BaseProperty* globalProperty = node->GetProperty(m_PropertyName.c_str());
  if (globalProperty)
  {
    PropertyChangedCommand::Pointer command = PropertyChangedCommand::New(this, node, 0);
    propertyObserverTags[0] = globalProperty->AddObserver(itk::ModifiedEvent(), command);
  }
  else
  {
    propertyObserverTags[0] = 0;
  }

  for (std::size_t i = 0; i < m_Renderers.size(); ++i)
  {
    /// Note:
    /// GetProperty() returns the global property if there is no renderer specific property.
    /// Therefore, we need to check if the property is really renderer specific.
    mitk::BaseProperty* rendererSpecificProperty = node->GetProperty(m_PropertyName.c_str(), m_Renderers[i]);
    if (rendererSpecificProperty && rendererSpecificProperty != globalProperty)
    {
      PropertyChangedCommand::Pointer command = PropertyChangedCommand::New(this, node, m_Renderers[i]);
      propertyObserverTags[i + 1] = rendererSpecificProperty->AddObserver(itk::ModifiedEvent(), command);
    }
    else
    {
      propertyObserverTags[i + 1] = 0;
    }
  }

  m_PropertyObserverTagsPerNode[node] = propertyObserverTags;
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::RemoveObservers(mitk::DataNode* node)
{
  NodePropertyObserverTags::iterator propertyObserverTagsPerNodeIt = m_PropertyObserverTagsPerNode.find(node);
  if (propertyObserverTagsPerNodeIt != m_PropertyObserverTagsPerNode.end())
  {
    std::vector<unsigned long>& propertyObserverTags = propertyObserverTagsPerNodeIt->second;
    mitk::BaseProperty* globalProperty = node->GetProperty(m_PropertyName.c_str(), 0);
    if (globalProperty)
    {
      globalProperty->RemoveObserver(propertyObserverTags[0]);
      propertyObserverTags[0] = 0;
    }

    for (std::size_t i = 0; i < m_Renderers.size(); ++i)
    {
      /// Note:
      /// GetProperty() returns the global property if there is no renderer specific property.
      /// Therefore, we need to check if the property is really renderer specific.
      mitk::BaseProperty* rendererSpecificProperty = node->GetProperty(m_PropertyName.c_str(), m_Renderers[i]);
      if (rendererSpecificProperty && rendererSpecificProperty != globalProperty)
      {
        rendererSpecificProperty->RemoveObserver(propertyObserverTags[i + 1]);
        propertyObserverTags[i + 1] = 0;
      }
    }

    m_PropertyObserverTagsPerNode.erase(node);
  }
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::AddAllObservers()
{
  mitk::DataStorage::Pointer dataStorage = this->GetDataStorage();
  if (dataStorage.IsNull())
  {
    return;
  }

  mitk::DataStorage::SetOfObjects::ConstPointer all = dataStorage->GetAll();
  for (mitk::DataStorage::SetOfObjects::ConstIterator it = all->Begin(); it != all->End(); ++it)
  {
    mitk::DataNode* node = it->Value();
    if (this->Pass(node))
    {
      this->AddObservers(node);
    }
  }
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::RemoveAllObservers()
{
  NodePropertyObserverTags::iterator propertyObserverTagsIt = m_PropertyObserverTagsPerNode.begin();
  NodePropertyObserverTags::iterator nodeToObserverTagsEnd = m_PropertyObserverTagsPerNode.end();

  for ( ; propertyObserverTagsIt != nodeToObserverTagsEnd; ++propertyObserverTagsIt)
  {
    mitk::DataNode* node = propertyObserverTagsIt->first;
    std::vector<unsigned long>& propertyObserverTags = propertyObserverTagsIt->second;
    if (propertyObserverTags.empty())
    {
      continue;
    }

    mitk::BaseProperty* globalProperty = node->GetProperty(m_PropertyName.c_str(), 0);
    if (globalProperty)
    {
      globalProperty->RemoveObserver(propertyObserverTags[0]);
    }

    for (std::size_t i = 0; i < m_Renderers.size(); ++i)
    {
      /// Note:
      /// GetProperty() returns the global property if there is no renderer specific property.
      /// Therefore, we need to check if the property is really renderer specific.
      mitk::BaseProperty* rendererSpecificProperty = node->GetProperty(m_PropertyName.c_str(), m_Renderers[i]);
      if (rendererSpecificProperty && rendererSpecificProperty != globalProperty)
      {
        rendererSpecificProperty->RemoveObserver(propertyObserverTags[i + 1]);
      }
    }
  }

  m_PropertyObserverTagsPerNode.clear();
}


//-----------------------------------------------------------------------------
void DataNodePropertyListener::OnPropertyChanged(mitk::DataNode* node, const mitk::BaseRenderer* renderer)
{
  if (!this->IsBlocked())
  {
    NodePropertyChanged.Send(node, renderer);
  }
}

}
