/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#if 1
  // MS VS
  #if defined(_MSC_VER)
    #define VIVID_TRAP() { if (IsDebuggerPresent()) { __debugbreak(); } else ::vl::abort_vl(); }
  // GNU GCC
  #elif defined(__GNUG__) || defined(__MINGW32__)
    #define VIVID_TRAP() { fflush(stdout); fflush(stderr); asm("int $0x3"); }
  #else
    #define VIVID_TRAP() { ::vl::abort_vl(); }
  #endif
  #define VIVID_CHECK(expr) { if(!(expr)) { ::vl::log_failed_check(#expr,__FILE__,__LINE__); VIVID_TRAP() } }
  #define VIVID_WARN(expr)  { if(!(expr)) { ::vl::log_failed_check(#expr,__FILE__,__LINE__); } }
#else
  #define VIVID_CHECK(expr) {}
  #define VIVID_WARN(expr) {}
  #define VIVID_TRAP() {}
#endif

#include <QTextStream>
#include <QFile>
#include <QDir>

#include "VLQtWidget.h"
#include <vlQt5/QtDirectory.hpp>
#include <vlQt5/QtFile.hpp>
#include <vlCore/Log.hpp>
#include <vlCore/Time.hpp>
#include <vlCore/Colors.hpp>
#include <vlCore/GlobalSettings.hpp>
#include <vlCore/FileSystem.hpp>
#include <vlCore/ResourceDatabase.hpp>
#include <vlGraphics/GeometryPrimitives.hpp>
#include <vlGraphics/RenderQueueSorter.hpp>
#include <vlGraphics/GLSL.hpp>
#include <vlGraphics/plugins/ioVLX.hpp>
#include <vlGraphics/FramebufferObject.hpp>
#include <vlGraphics/AdjacencyExtractor.hpp>
#include <vlVolume/RaycastVolume.hpp>
#include <cassert>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkLinearTransform.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkPolyDataNormals.h>
#include <vtkImageData.h>
#include <mitkProperties.h>
#include <mitkImageReadAccessor.h>
#include <mitkDataStorage.h>
#include <mitkImage.h>
#include <stdexcept>
#include <sstream>
#include <niftkScopedOGLContext.h>
#include "TrackballManipulator.h"
#ifdef BUILD_IGI
#include <CameraCalibration/niftkUndistortion.h>
#include <mitkCameraIntrinsicsProperty.h>
#include <mitkCameraIntrinsics.h>
#endif
#include <mitkCoordinateAxesData.h>

#ifdef _USE_PCL
#include <niftkPCLData.h>
#endif

#ifdef _MSC_VER
#ifdef _USE_NVAPI
#include <nvapi.h>
#endif
#endif

//-----------------------------------------------------------------------------
// CUDA stuff
//-----------------------------------------------------------------------------

#ifdef _USE_CUDA

  #include <Rendering/VLFramebufferToCUDA.h>
  #include <niftkCUDAManager.h>
  #include <niftkCUDAImage.h>
  #include <niftkLightweightCUDAImage.h>
  #include <niftkCUDAImageProperty.h>
  #include <niftkFlipImageLauncher.h>
  #include <cuda_gl_interop.h>

  //-----------------------------------------------------------------------------

  struct CUDAInterop
  {
    std::string m_NodeName;
    mitk::DataStorage::Pointer m_DataStorage;

    VLFramebufferAdaptor* m_FBOAdaptor;

    CUDAInterop() : m_FBOAdaptor(0)
    {
    }

    ~CUDAInterop()
    {
      delete m_FBOAdaptor;
    }
  };

  //-----------------------------------------------------------------------------

  VLQtWidget::TextureDataPOD::TextureDataPOD() : m_LastUpdatedID(0) , m_CUDARes(0)
  {
  }

// #else
//   struct CUDAInterop { };
#endif

using namespace vl;

//-----------------------------------------------------------------------------
// Util functions
//-----------------------------------------------------------------------------

namespace
{
  void dumpNodeInfo( const std::string& prefix, const mitk::DataNode::ConstPointer& node ) {
    printf( "\n%s: ", prefix.c_str() );
    const char* class_name = node->GetData() ? node->GetData()->GetNameOfClass() : "<unknown-class>";
    mitk::StringProperty* name_prop = dynamic_cast<mitk::StringProperty*>(node->GetProperty("name"));
    const char* object_name2 = "<unknown-name>";
    if (name_prop != 0) {
      object_name2 = name_prop->GetValue();
    }
    printf( "%s <%s>\n", object_name2, class_name );

    const mitk::PropertyList::PropertyMap* propList = node->GetPropertyList()->GetMap();
    mitk::PropertyList::PropertyMap::const_iterator it = node->GetPropertyList()->GetMap()->begin();
    for( ; it != node->GetPropertyList()->GetMap()->end(); ++it ) {
      const std::string name = it->first;
      const mitk::BaseProperty::Pointer prop = it->second;
      printf( "\t%s: %s <%s>\n", name.c_str(), prop->GetValueAsString().c_str(), prop->GetNameOfClass() );
      if ( name == "size" ) {
        VIVID_CHECK( node->GetProperty( "size") );
      }
    }
  }
}

//-----------------------------------------------------------------------------
// Init and shutdown VL
//-----------------------------------------------------------------------------

namespace
{
  class VLInit
  {
  public:
    VLInit() { vl::VisualizationLibrary::init(); }
    ~VLInit() { vl::VisualizationLibrary::shutdown(); }
  };

  VLInit s_ModuleInit;
}

//-----------------------------------------------------------------------------
// VLUserData
//-----------------------------------------------------------------------------

struct VLUserData : public vl::Object
{
  VLUserData()
    : m_TransformModifiedTime(0)
    , m_ImageModifiedTime(0)
  {
  }

  itk::ModifiedTimeType m_TransformModifiedTime;
  itk::ModifiedTimeType m_ImageModifiedTime;
};

//-----------------------------------------------------------------------------
// VLNode
//-----------------------------------------------------------------------------

/** Takes care of managing all VL related aspects with regard to a given mitk::DataNode. */
class VLNode: public vl::Object {
public:
  VLNode( vl::VividRendering* vr, mitk::DataStorage* ds, mitk::DataNode* node, vl::OpenGLContext* gl ) {
    // Init
    m_OpenGLContext = gl;
    m_VividRendering = vr;
    m_DataStorage = ds;
    m_DataNode = node;
    // Activate OpenGL context
    gl->makeCurrent();
    // Initialize properties
    initDataStoreProperties( node );
  }

  /** Updates all the relevant VL data structures, uniforms etc. according to the node's settings. */
  virtual void update() = 0;

  /** Removes all the relevant Actor(s) from the scene. */
  virtual void remove() = 0;

  /** Factory method: creates the right VLNode subclass according to the node's type. */
  static ref<VLNode> create( const mitk::DataNode* node ) {
    return NULL;
  }

private:
  /** Initializes the value of all Vivid properties in the DataStore. */
  void initDataStoreProperties( mitk::DataNode* node ) {
    // MIC FIXME:
    // ...
  }

protected:
  vl::OpenGLContext* m_OpenGLContext;
  vl::VividRendering* m_VividRendering;
  mitk::DataStorage* m_DataStorage;
  mitk::DataNode* m_DataNode;
};

//-----------------------------------------------------------------------------
// VLQtWidget
//-----------------------------------------------------------------------------

VLQtWidget::VLQtWidget(QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(parent, shareWidget, f)
  , m_BackgroundWidth( 0 )
  , m_BackgroundHeight( 0 )
  , m_ScheduleTrackballAdjustView( true )
  , m_Refresh(1000 / 30) // 30 fps
  , m_OclService(0)
#ifdef _USE_CUDA
  , m_CUDAInteropPimpl(0)
#endif
{
  setContinuousUpdate(false);
  setMouseTracking(true);
  setAutoBufferSwap(false);
  setAcceptDrops(false);
  // let Qt take care of object destruction.
  vl::OpenGLContext::setAutomaticDelete(false);
}

//-----------------------------------------------------------------------------

VLQtWidget::~VLQtWidget()
{
  makeCurrent();

  RemoveDataStorageListeners();

  dispatchDestroyEvent();

#ifdef _USE_CUDA
  FreeCUDAInteropTextures();
#endif
}

//-----------------------------------------------------------------------------

void VLQtWidget::AddDataStorageListeners()
{
  if (m_DataStorage.IsNotNull())
  {
    m_DataStorage->AddNodeEvent.    AddListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeAdd));
    m_DataStorage->ChangedNodeEvent.AddListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeUpdate));
    m_DataStorage->RemoveNodeEvent. AddListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeRemove));
    m_DataStorage->DeleteNodeEvent. AddListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeRemove));
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::RemoveDataStorageListeners()
{
  if (m_DataStorage.IsNotNull())
  {
    m_DataStorage->AddNodeEvent.    RemoveListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeAdd));
    m_DataStorage->ChangedNodeEvent.RemoveListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeUpdate));
    m_DataStorage->RemoveNodeEvent. RemoveListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeRemove));
    m_DataStorage->DeleteNodeEvent. RemoveListener(mitk::MessageDelegate1<VLQtWidget, const mitk::DataNode*>(this, &VLQtWidget::ScheduleNodeRemove));
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::SetDataStorage(const mitk::DataStorage::Pointer& dataStorage)
{
  makeCurrent();

  RemoveDataStorageListeners();

#ifdef _USE_CUDA
  FreeCUDAInteropTextures();
#endif

  m_DataStorage = dataStorage;
  AddDataStorageListeners();

  ClearScene();

  update();
}

//-----------------------------------------------------------------------------

void VLQtWidget::SetOclResourceService(OclResourceService* oclserv)
{
 // no idea if this is really a necessary restriction.
 // if it is then maybe the ocl-service should be a constructor parameter.
 if (m_OclService != 0)
   throw std::runtime_error("Can set OpenCL service only once");

 m_OclService = oclserv;
}

//-----------------------------------------------------------------------------

void VLQtWidget::ScheduleNodeAdd( const mitk::DataNode* node )
{
  if ( ! node || ! node->GetData() ) {
    return;
  }

  // m_NodesToRemove.erase( node ); // remove it first
  m_NodesToAdd.insert( mitk::DataNode::ConstPointer ( node ) ); // then add
  // m_NodesToUpdate.erase( node ); // then update
  update();

  const char* noc = node->GetData() ? node->GetData()->GetNameOfClass() : "<name-of-class>";
  printf("ScheduleNodeAdd: %s (%s)\n", node->GetName().c_str(), noc );
}

//-----------------------------------------------------------------------------

void VLQtWidget::ScheduleNodeUpdate( const mitk::DataNode* node )
{
  if ( ! node || ! node->GetData() ) {
    return;
  }

  m_NodesToRemove.erase( node ); // abort the removal
  // m_NodesToAdd.erase( node ); // let it add it first
  m_NodesToUpdate.insert( mitk::DataNode::ConstPointer ( node ) ); // then update
  update();

  const char* noc = node->GetData() ? node->GetData()->GetNameOfClass() : "<unknown-class>";
  printf("ScheduleNodeUpdate: %s (%s)\n", node->GetName().c_str(), noc );
}

//-----------------------------------------------------------------------------

void VLQtWidget::ScheduleNodeRemove( const mitk::DataNode* node )
{
  if ( ! node /* || ! node->GetData() */ ) {
    return;
  }

  m_NodesToRemove.insert( mitk::DataNode::ConstPointer ( node ) ); // remove it
  m_NodesToAdd.erase( node );    // abort the addition
  m_NodesToUpdate.erase( node ); // abort the update
  update();

  const char* noc = node->GetData() ? node->GetData()->GetNameOfClass() : "<name-of-class>";
  printf("ScheduleNodeRemove: %s (%s)\n", node->GetName().c_str(), noc );
}

//-----------------------------------------------------------------------------

void VLQtWidget::InitSceneFromDataStorage()
{
  // Make sure the system is initialized
  VIVID_CHECK( m_VividRendering.get() );

  makeCurrent();

  ClearScene();

  if ( m_DataStorage.IsNull() ) {
    return;
  }

  typedef itk::VectorContainer<unsigned int, mitk::DataNode::Pointer> NodesContainerType;
  NodesContainerType::ConstPointer vc = m_DataStorage->GetAll();

  for (unsigned int i = 0; i < vc->Size(); ++i)
  {
    mitk::DataNode::Pointer currentDataNode = vc->ElementAt(i);
    if (currentDataNode.IsNull() || currentDataNode->GetData()== 0) {
      continue;
    } else {
      AddDataNode( mitk::DataNode::ConstPointer( currentDataNode.GetPointer() ) );
    }
  }

  #if 0
    // dump scene to VLB/VLT format
    ref< vl::ResourceDatabase > db = new vl::ResourceDatabase;
    for( int i = 0; i < m_SceneManager->tree()->actors()->size(); ++i ) {
      vl::Actor* act = m_SceneManager->tree()->actors()->at(i);
      if ( act->enableMask() ) {
        // db->resources().push_back( act );
        // vl::String fname = filename( files[i] );
        db->resources().push_back( act );
        vl::String fname = "niftk-liver";
        vl::saveVLT( "C:/git-ucl/VisualizationLibrary/data/tmp/" + fname + ".vlt", db.get() );
        vl::saveVLB( "C:/git-ucl/VisualizationLibrary/data/tmp/" + fname + ".vlb", db.get() );
      }
    }
  #endif
}

//-----------------------------------------------------------------------------

void VLQtWidget::AddDataNode(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  if ( node.IsNull() || node->GetData() == 0 ) {
    return;
  }

  // only add node once.
  if ( GetNodeActor( node ) ) {
    return;
  }

  #if 0
    dumpNodeInfo( "AddDataNode()", node );
  #endif
  // Propagate color and opacity down to basedata
  node->GetData()->SetProperty("color", node->GetProperty("color"));
  node->GetData()->SetProperty("opacity", node->GetProperty("opacity"));
  node->GetData()->SetProperty("visible", node->GetProperty("visible"));

  bool doMitkImageIfSuitable = true;
  mitk::Image::Pointer              mitkImg  = dynamic_cast<mitk::Image*>(node->GetData());
  mitk::Surface::Pointer            mitkSurf = dynamic_cast<mitk::Surface*>(node->GetData());
  mitk::PointSet::Pointer           mitkPS   = dynamic_cast<mitk::PointSet*>(node->GetData());
#ifdef _USE_PCL
  niftk::PCLData::Pointer           pclPS    = dynamic_cast<niftk::PCLData*>(node->GetData());
#endif
  mitk::CoordinateAxesData::Pointer coords   = dynamic_cast<mitk::CoordinateAxesData*>(node->GetData());
#ifdef _USE_CUDA
  mitk::BaseData::Pointer           cudaImg  = dynamic_cast<niftk::CUDAImage*>(node->GetData());
  // this check will prefer a CUDAImageProperty attached to the node's data object.
  // e.g. if there is mitk::Image and an attached CUDAImageProperty then CUDAImageProperty wins and
  // mitk::Image is ignored.
  doMitkImageIfSuitable = dynamic_cast<niftk::CUDAImageProperty*>( node->GetData()->GetProperty("CUDAImageProperty").GetPointer() ) == 0;
  if (doMitkImageIfSuitable == false)
  {
    cudaImg = node->GetData();
  }
#endif

  ref<vl::Actor> actor;
  std::string actor_name;

  if (mitkSurf.IsNotNull())
  {
    actor = AddSurfaceActor(mitkSurf);
    actor_name = "surface:";
  }
  else
  if (mitkImg.IsNotNull() && doMitkImageIfSuitable)
  {
    actor = AddImageActor(mitkImg);
    actor_name = "image:";
  }
  else
  if (mitkPS.IsNotNull())
  {
    actor = AddPointsetActor(mitkPS);
    actor_name = "point-set:";
  }
  else
  if (coords.IsNotNull())
  {
    actor = AddCoordinateAxisActor(coords);
    actor_name = "coordinate-axis-data:";
  }
#ifdef _USE_PCL
  else
  if (pclPS.IsNotNull())
  {
    actor = AddPointCloudActor(pclPS);
    actor_name = "pcl:";
  }
#endif
#ifdef _USE_CUDA
  else
  if (cudaImg.IsNotNull())
  {
    actor = AddCUDAImageActor(cudaImg);
    actor_name = "cuda-image:";

    m_NodeToTextureMap[node] = TextureDataPOD();
  }
#endif

  if ( actor ) {
    // Set object name
    std::string node_name;
    node->GetStringProperty( "name", node_name );
    actor_name += node_name;
    actor->setObjectName( actor_name );

    // Populate Node/Actor map
    m_NodeActorMap[node] = actor;

    // Initialize Actor properties based on Node data (color, visibility, transform, image etc.)
    UpdateDataNode(node);
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::RemoveDataNode(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  // dont leave a dangling update behind.
  m_NodesToUpdate.erase(node);
  m_NodesToAdd.erase(node);

  if ( ! node || ! node->GetData() ) {
    return;
  }

#ifdef _USE_CUDA
  {
    std::map<mitk::DataNode::ConstPointer, TextureDataPOD>::iterator i = m_NodeToTextureMap.find(node);
    if (i != m_NodeToTextureMap.end())
    {
      if (i->second.m_CUDARes != 0)
      {
        cudaError_t err = cudaGraphicsUnregisterResource(i->second.m_CUDARes);
        if (err != cudaSuccess)
        {
          MITK_WARN << "Failed to unregister VL texture from CUDA";
        }
      }

      m_NodeToTextureMap.erase(i);
    }
  }
#endif

  // Remove Node/Actor couple from map & remove Actor from scene
  NodeActorMapType::iterator it = m_NodeActorMap.find( node );
  if ( it != m_NodeActorMap.end() ) {
    vl::Actor* actor = it->second.get();
    if ( actor ) {
      m_SceneManager->tree()->eraseActor( actor );
      m_NodeActorMap.erase(it);
    }
  }
}

void VLQtWidget::UpdateDataNode(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  if ( node.IsNull() || node->GetData() == 0 ) {
    return;
  }

  #if 1
    dumpNodeInfo( "UpdateDataNode()", node );
  #endif

  vl::Actor* actor = GetNodeActor(node);
  if ( ! actor ) {
    return;
  }

  vl::Shader* shader = actor->effect()->shader();

  // Update visibility
  bool visible = true;
  mitk::BoolProperty* visibleProp = dynamic_cast<mitk::BoolProperty*>(node->GetProperty("visible"));
  if ( visibleProp ) {
    visible = visibleProp->GetValue();
  }
  float opacity = 1.0f;
  mitk::FloatProperty* opacityProp = dynamic_cast<mitk::FloatProperty*>(node->GetProperty("opacity"));
  if ( opacityProp ) {
    opacity = opacityProp->GetValue();
    visible &= opacity > 1.0f / 255.0f;
  }
  VIVID_CHECK( actor->enableMask() == vl::VividRenderer::DefaultEnableMask ||
               actor->enableMask() == vl::VividRenderer::VolumeEnableMask  );
  actor->setEnabled( visible );

  // Update color and opacity
  vl::fvec4 color(1, 1, 1, opacity);
  mitk::ColorProperty* colorProp = dynamic_cast<mitk::ColorProperty*>(node->GetProperty("color"));
  if ( colorProp ) {
    mitk::Color mitkColor = colorProp->GetColor();
    color.r() = mitkColor.GetRed();
    color.g() = mitkColor.GetGreen();
    color.b() = mitkColor.GetBlue();
  }
  shader->gocMaterial()->setDiffuse( color );

  // Update point size
  float pointsize = 1;
  node->GetFloatProperty("pointsize", pointsize);
  // this is part of the standard vivid shader so it must be present.
  VIVID_CHECK( shader->getPointSize() );
  shader->getPointSize()->set( pointsize );
  pointsize > 1 ? shader->enable(vl::EN_POINT_SMOOTH) :
                  shader->disable(vl::EN_POINT_SMOOTH);

  // Update Actor's Transform
  UpdateActorTransformFromNode(actor, node);

  // Update texture
  UpdateTextureFromImage(node);

  // Update camera
  if (node == m_CameraNode) {
    UpdateCameraParameters();
  }

  // if we do have live-updating textures then we do need to refresh the vl-side of it!
  // even if the node is not visible.
#ifdef _USE_CUDA
  UpdateGLTexturesFromCUDA(node);
#endif
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddSurfaceActor(const mitk::Surface::Pointer& mitkSurf)
{
  makeCurrent();

  // MITK_INFO << "Num of vertices: " << vlSurf->vertexArray()->size()/3;

  ref<vl::Geometry> geom = ConvertVTKPolyData(mitkSurf->GetVtkPolyData());
  if ( ! geom->normalArray() ) {
    geom->computeNormals();
  }

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();
  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), mitkSurf.GetPointer());
  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(geom.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );
  return actor;
}

//-----------------------------------------------------------------------------

ref<vl::Geometry> VLQtWidget::ConvertVTKPolyData(vtkPolyData* vtkPoly)
{
  makeCurrent();

  if (vtkPoly == 0)
    return NULL;

  ref<vl::Geometry> vlPoly = new vl::Geometry;

  // Buffer in host memory to store cell info
  unsigned int* m_IndexBuffer = 0;

  // Buffer in host memory to store vertex points
  float* m_PointBuffer = 0;

  // Buffer in host memory to store normals associated with vertices
  float* m_NormalBuffer = 0;

  // Buffer in host memory to store scalar info associated with vertices
  char* m_ScalarBuffer = 0;

  unsigned int numOfvtkPolyPoints = vtkPoly->GetNumberOfPoints();

  // A polydata will always have point data
  int pointArrayNum = vtkPoly->GetPointData()->GetNumberOfArrays();

  if (pointArrayNum == 0 && numOfvtkPolyPoints == 0)
  {
    MITK_ERROR << "No points detected in the vtkPoly data!\n";
    return NULL;
  }

  // We'll have to build the cell data if not present already
  int cellArrayNum  = vtkPoly->GetCellData()->GetNumberOfArrays();
  if ( cellArrayNum == 0 ) {
    vtkPoly->BuildCells();
  }

  vtkSmartPointer<vtkCellArray> verts;

  // Try to get access to cells
  if (vtkPoly->GetVerts() != 0 && vtkPoly->GetVerts()->GetNumberOfCells() != 0)
    verts = vtkPoly->GetVerts();
  else if (vtkPoly->GetLines() != 0 && vtkPoly->GetLines()->GetNumberOfCells() != 0)
    verts = vtkPoly->GetLines();
  else if (vtkPoly->GetPolys() != 0 && vtkPoly->GetPolys()->GetNumberOfCells() != 0)
    verts = vtkPoly->GetPolys();
  else if (vtkPoly->GetStrips() != 0 && vtkPoly->GetStrips()->GetNumberOfCells() != 0)
    verts = vtkPoly->GetStrips();

  if (verts->GetMaxCellSize() > 3)
  {
    // Panic and return
    MITK_ERROR << "More than three vertices / cell detected, can't handle this data type!\n";
    return NULL;
  }

  vtkSmartPointer<vtkPoints> points = vtkPoly->GetPoints();

  if (points == 0)
  {
    MITK_ERROR << "Corrupt vtkPoly, returning! \n";
    return NULL;
  }

  // Deal with normals
  vtkSmartPointer<vtkDataArray> normals = vtkPoly->GetPointData()->GetNormals();

  if (normals == 0)
  {
    MITK_INFO << "Generating normals for the vtkPoly data (mitk::OclSurface)";

    vtkSmartPointer<vtkPolyDataNormals> normalGen = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalGen->SetInputData(vtkPoly);
    normalGen->AutoOrientNormalsOn();
    normalGen->Update();

    normals = normalGen->GetOutput()->GetPointData()->GetNormals();

    if (normals == 0)
    {
      MITK_ERROR << "Couldn't generate normals, returning! \n";
      return NULL;
    }

    vtkPoly->GetPointData()->SetNormals(normals);
    vtkPoly->GetPointData()->GetNormals()->Modified();
    vtkPoly->GetPointData()->Modified();
  }

  // Check if we have scalars
  vtkSmartPointer<vtkDataArray> scalars = vtkPoly->GetPointData()->GetScalars();

  bool pointsValid  = (points.GetPointer() == 0) ? false : true;
  bool normalsValid = (normals.GetPointer() == 0) ? false : true;
  bool scalarsValid = (scalars.GetPointer() == 0) ? false : true;

  unsigned int pointBufferSize = 0;
  unsigned int numOfPoints = static_cast<unsigned int> (points->GetNumberOfPoints());
  pointBufferSize = numOfPoints * sizeof(float) * 3;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Deal with points

  // Allocate memory
  m_PointBuffer = new float[numOfPoints*3];

  // Copy data to buffer
  memcpy(m_PointBuffer, points->GetVoidPointer(0), pointBufferSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Deal with normals

  if (normalsValid)
  {
    // Get the number of normals we have to deal with
    int m_NormalCount = static_cast<unsigned int> (normals->GetNumberOfTuples());
    VIVID_CHECK(m_NormalCount == numOfPoints);

    // Size of the buffer that is required to store all the normals
    unsigned int normalBufferSize = numOfPoints * sizeof(float) * 3;

    // Allocate memory
    m_NormalBuffer = new float[numOfPoints*3];

    // Copy data to buffer
    memcpy(m_NormalBuffer, normals->GetVoidPointer(0), normalBufferSize);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Deal with scalars (colors or texture coordinates)
  if (scalarsValid)
  {

    // Get the number of scalars we have to deal with
    int m_ScalarCount = static_cast<unsigned int> (scalars->GetNumberOfTuples());

    // Size of the buffer that is required to store all the scalars
    unsigned int scalarBufferSize = numOfPoints * sizeof(char) * 1;

    // Allocate memory
    m_ScalarBuffer = new char[numOfPoints];

    // Copy data to buffer
    memcpy(m_ScalarBuffer, scalars->GetVoidPointer(0), scalarBufferSize);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Deal with cells - initialize index buffer
  vtkIdType npts;
  vtkIdType *pts;

  // Get the number of indices we have to deal with
  unsigned int m_IndexCount = static_cast<unsigned int> (verts->GetNumberOfCells());

  // Get the max number of vertices / cell
  int maxPointsPerCell = verts->GetMaxCellSize();

  // Get the number of indices we have to deal with
  unsigned int numOfTriangles = static_cast<unsigned int> (verts->GetNumberOfCells());

  // Allocate memory for the index buffer
  m_IndexBuffer = new unsigned int[numOfTriangles*3];
  memset(m_IndexBuffer, 0, numOfTriangles*3*sizeof(unsigned int));

  verts->InitTraversal();

  unsigned int cellIndex = 0;
  // Iterating through all the cells
  while (cellIndex < numOfTriangles)
  {
    verts->GetNextCell(npts, pts);

    // Copy the indices into the index buffer
    for (size_t i = 0; i < static_cast<size_t>(npts); i++)
      m_IndexBuffer[cellIndex*3 +i] = pts[i];

    cellIndex++;
  }
  MITK_INFO << "Surface data initialized. Num of Points: " <<points->GetNumberOfPoints() << " Num of Cells: " <<verts->GetNumberOfCells() << "\n";

  ref<vl::ArrayFloat3>  vl_verts   = new vl::ArrayFloat3;
  ref<vl::ArrayFloat3>  vlNormals = new vl::ArrayFloat3;
  ref<vl::DrawElementsUInt> vlTriangles = new vl::DrawElementsUInt(vl::PT_TRIANGLES);

  vl_verts->resize(numOfPoints * 3);
  vlNormals->resize(numOfPoints * 3);

  vlPoly->drawCalls().push_back(vlTriangles.get());
  vlTriangles->indexBuffer()->resize(numOfTriangles*3);

  vlPoly->setVertexArray(vl_verts.get());
  vlPoly->setNormalArray(vlNormals.get());

  float* vertBufFlotPtr = reinterpret_cast<float *>(vl_verts->ptr());
  float* normBufFlotPtr = reinterpret_cast<float *>(vlNormals->ptr());

  // Vertices and normals
  for (unsigned int i=0; i<numOfPoints; ++i)
  {
    vertBufFlotPtr[3*i + 0] = m_PointBuffer[i*3 +0];
    vertBufFlotPtr[3*i + 1] = m_PointBuffer[i*3 +1];
    vertBufFlotPtr[3*i + 2] = m_PointBuffer[i*3 +2];

    normBufFlotPtr[3*i + 0] = m_NormalBuffer[i*3 +0];
    normBufFlotPtr[3*i + 1] = m_NormalBuffer[i*3 +1];
    normBufFlotPtr[3*i + 2] = m_NormalBuffer[i*3 +2];
  }

  // Make sure that the values are copied onto GPU memory
  //vlPoly->vertexArray()->updateBufferObject();
  //glFinish();

  // Read triangles
  for(unsigned int i=0; i<numOfTriangles; ++i)
  {
    vlTriangles->indexBuffer()->at(i*3+0) = m_IndexBuffer[i*3 +0];
    vlTriangles->indexBuffer()->at(i*3+1) = m_IndexBuffer[i*3 +1];
    vlTriangles->indexBuffer()->at(i*3+2) = m_IndexBuffer[i*3 +2];
  }

  // Make sure that the values are copied onto GPU memory
  vl_verts->updateBufferObject();
  vlNormals->updateBufferObject();
  vlTriangles->indexBuffer()->updateBufferObject();
  glFinish();

  // Buffer in host memory to store cell info
  if (m_IndexBuffer != 0)
    delete m_IndexBuffer;

  // Buffer in host memory to store vertex points
  if (m_PointBuffer != 0)
    delete m_PointBuffer;

  // Buffer in host memory to store normals associated with vertices
  if (m_NormalBuffer != 0)
    delete m_NormalBuffer;

  // Buffer in host memory to store scalar info associated with vertices
  if (m_ScalarBuffer != 0)
    delete m_ScalarBuffer;

  // MITK_INFO << "Num of VL vertices: " << vlPoly->vertexArray()->size() / 3;

  // Finally convert to adjacency format so we can render silhouettes etc.
  return vl::AdjacencyExtractor::extract( vlPoly.get() );
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddImageActor(const mitk::Image::Pointer& mitkImg)
{
  makeCurrent();

  unsigned int* dims = 0;
  dims = mitkImg->GetDimensions();
  // we do not own dims!

  if (dims[2] <= 1)
  {
    return Add2DImageActor(mitkImg);
  }
  else
  {
    return Add3DImageActor(mitkImg);
  }
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::Add2DImageActor(const mitk::Image::Pointer& mitkImg)
{
  makeCurrent();

  mitk::PixelType  mitk_pixel_type = mitkImg->GetPixelType();
  vl::EImageType   vl_type         = MapITKPixelTypeToVL(mitk_pixel_type.GetComponentType());
  vl::EImageFormat vl_format       = MapComponentsToVLColourFormat(mitk_pixel_type.GetNumberOfComponents());
  unsigned int*    dims            = mitkImg->GetDimensions();

  ref<vl::Image> vl_img = new vl::Image(dims[0], dims[1], 0, 1, vl_format, vl_type);

  try
  {
    unsigned int byte_count = dims[0] * dims[1] * dims[2] * mitk_pixel_type.GetSize();
    VIVID_CHECK( vl_img->requiredMemory() == byte_count );
    mitk::ImageReadAccessor readAccess( mitkImg, mitkImg->GetVolumeData(0) );
    const void* ptr = readAccess.GetData();
    std::memcpy( vl_img->pixels(), ptr, byte_count );
  }
  catch (...)
  {
    // FIXME: error handling?
    MITK_ERROR << "Did not get pixel read access to 2D image.";
  }

  ref<vl::Geometry> geom = CreateGeometryFor2DImage(dims[0], dims[1]);

  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), mitkImg.GetPointer());

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();
  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(geom.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );

  fx->shader()->getUniform("vl_Vivid.enableTextureMapping")->setUniformI( 1 );
  fx->shader()->getUniform("vl_Vivid.enableLighting")->setUniformI( 0 );
  // When texture mapping is enabled texture is modulated by vertex color
  geom->setColorArray( vl::white );
  // These must be present as part of the default Vivid material
  VIVID_CHECK( fx->shader()->getTextureSampler( vl::VividRendering::UserTexture ) )
  VIVID_CHECK( fx->shader()->getTextureSampler( vl::VividRendering::UserTexture )->texture() )
  VIVID_CHECK( fx->shader()->getUniform("vl_UserTexture")->getUniformI() == vl::VividRendering::UserTexture );
  ref<vl::Texture> texture = fx->shader()->getTextureSampler( vl::VividRendering::UserTexture )->texture();

  // I think we can ignore the format
  if ( vl_img->width() != texture->width() || vl_img->height() != texture->height() ) {
    // Recreate new texture (TexParameter is not reset so we can keep the current defaults)
    texture->destroyTexture();
    texture->createTexture2D( vl_img.get(), vl::TF_UNKNOWN, false, false );
  } else {
    // Update the texture
    texture->setMipLevel( 0, vl_img.get(), false );
  }
  return actor;
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::Add3DImageActor(const mitk::Image::Pointer& mitkImg)
{
  // MIC FIXME:

  throw std::runtime_error("VLQtWidget::Add3DImageActor(): to be implemented!");

  makeCurrent();

  mitk::PixelType mitk_pixel_type = mitkImg->GetPixelType();
  size_t numOfComponents = mitk_pixel_type.GetNumberOfComponents();

  if (false)
  {
    std::cout << "Original pixel type:" << std::endl;
    std::cout << " PixelType: " <<mitk_pixel_type.GetTypeAsString() << std::endl;
    std::cout << " BitsPerElement: " <<mitk_pixel_type.GetBpe() << std::endl;
    std::cout << " NumberOfComponents: " << numOfComponents << std::endl;
    std::cout << " BitsPerComponent: " <<mitk_pixel_type.GetBitsPerComponent() << std::endl;
  }

  ref<vl::Image> vl_img;

  try
  {
    mitk::ImageReadAccessor readAccess(mitkImg, mitkImg->GetVolumeData(0));
    const void* cPointer = readAccess.GetData();

    vl::EImageType     type = MapITKPixelTypeToVL(mitk_pixel_type.GetComponentType());
    vl::EImageFormat   format;

    if (type != vl::IT_FLOAT)
    {
      if (numOfComponents == 1)
        format = vl::IF_LUMINANCE;
      else if (numOfComponents == 2)
        format = vl::IF_RG_INTEGER;
      else if (numOfComponents == 3)
        format = vl::IF_RGB_INTEGER;
      else if (numOfComponents == 4)
        // FIXME: not sure whether we really want integer formats here!
        //        for now, dont do integer for rgba, we have quite a few rgba images.
        format = vl::IF_RGBA;//_INTEGER;
    }
    else if (type == vl::IT_FLOAT)
    {
      if (numOfComponents == 1)
        format = vl::IF_LUMINANCE;
      else if (numOfComponents == 2)
        format = vl::IF_RG;
      else if (numOfComponents == 3)
        format = vl::IF_RGB;
      else if (numOfComponents == 4)
        format = vl::IF_RGBA;
    }

    unsigned int* dims = 0;
    dims = mitkImg->GetDimensions();
    // we do not own dims!

    int bytealign = 1;
    if (dims[2] <= 1)
      vl_img = new vl::Image(dims[0], dims[1], 0, bytealign, format, type);
    else
      vl_img = new vl::Image(dims[0], dims[1], dims[2], bytealign, format, type);

    // sanity check
    unsigned int size = (dims[0] * dims[1] * dims[2]) * mitk_pixel_type.GetSize();
    VIVID_CHECK(vl_img->requiredMemory() == size);
    std::memcpy(vl_img->pixels(), cPointer, vl_img->requiredMemory());

    vl_img = vl_img->convertFormat(vl::IF_LUMINANCE)->convertType(vl::IT_UNSIGNED_SHORT);
/*
    ref<KeyValues> tags = new KeyValues;
    tags->set("Origin")    = Say("%n %n %n") << mitkImg->GetGeometry()->GetOrigin()[0]  << mitkImg->GetGeometry()->GetOrigin()[1]  << mitkImg->GetGeometry()->GetOrigin()[2];
    tags->set("Spacing")   = Say("%n %n %n") << mitkImg->GetGeometry()->GetSpacing()[0] << mitkImg->GetGeometry()->GetSpacing()[1] << mitkImg->GetGeometry()->GetSpacing()[2];
    vl_img->setTags(tags.get());
*/
  }
  catch(mitk::Exception& e)
  {
    // deal with the situation not to have access
    VIVID_CHECK(false);
  }

  float opacity;
  mitkImg->GetPropertyList()->GetFloatProperty("opacity", opacity);

  mitk::BaseProperty::Pointer   colourProp = mitkImg->GetProperty("color");
  mitk::Color                   mitkColor;
  if (colourProp.IsNotNull())
    mitkColor = dynamic_cast<mitk::ColorProperty*>(colourProp.GetPointer())->GetColor();

  vl::fvec4 color;
  color[0] = mitkColor[0];
  color[1] = mitkColor[1];
  color[2] = mitkColor[2];
  color[3] = opacity;

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();
  fx->shader()->enable(vl::EN_DEPTH_TEST);
  fx->shader()->enable(vl::EN_BLEND);
  // fx->shader()->setRenderState(m_Light.get(), 0);
  fx->shader()->enable(vl::EN_LIGHTING);
  fx->shader()->gocMaterial()->setDiffuse(color);
  fx->shader()->gocMaterial()->setTransparency(opacity);

  //vl::String fragmentShaderSource   = LoadGLSLSourceFromResources("volume_raycast_isosurface_transp.fs");
  //vl::String vertexShaderSource     = LoadGLSLSourceFromResources("volume_luminance_light.vs");

  //// The GLSL program used to perform the actual rendering.
  //// The \a volume_luminance_light.fs fragment shader allows you to specify how many
  //// lights to use (up to 4) and can optionally take advantage of a precomputed normals texture.
  //ref<vl::GLSLProgram>    glslShader = fx->shader()->gocGLSLProgram();
  //glslShader->attachShader(new vl::GLSLFragmentShader(fragmentShaderSource));
  //glslShader->attachShader(new vl::GLSLVertexShader(vertexShaderSource));

  ref<vl::Actor> imageActor = new vl::Actor;
  imageActor->setEffect(fx.get());
  // imageActor->setUniform(m_ThresholdVal.get());

  ref<vl::Transform>    tr = new vl::Transform;
  //UpdateTransfromFromData(tr, cudaImg);       // FIXME: needs proper thinking through
  imageActor->setTransform(tr.get());
  m_SceneManager->tree()->addActor(imageActor.get());
  imageActor->setEnableMask( vl::VividRenderer::VolumeEnableMask );

  // this is a callback: gets triggered everytime its bound actor is to be rendered.
  // during that callback it updates the uniforms of our glsl shader to match fixed-function state.
  ref<vl::RaycastVolume>    raycastVolume = new vl::RaycastVolume;
  // this stuffs the proxy geometry onto our actor, as lod-slot zero.
  raycastVolume->bindActor(imageActor.get());

  // we do not own dims!
  unsigned int*   dims    = mitkImg->GetDimensions();
  mitk::Vector3D  spacing = mitkImg->GetGeometry()->GetSpacing();

  float dimX = (float) dims[0] * spacing[0] / 2.0f;
  float dimY = (float) dims[1] * spacing[1] / 2.0f;
  float dimZ = (float) dims[2] * spacing[2] / 2.0f;
  float shiftX = 0.0f;//0.5f * spacing[0];
  float shiftY = 0.0f;//0.5f * spacing[1];
  float shiftZ = 0.0f;//0.5f * spacing[2];

  vl::AABB volume_box(vl::vec3(-dimX + shiftX, -dimY + shiftY, -dimZ + shiftZ)
                       , vl::vec3( dimX + shiftX,  dimY + shiftY,  dimZ + shiftZ));
  raycastVolume->setBox(volume_box);
  raycastVolume->generateTextureCoordinates(vl::ivec3(vl_img->width(), vl_img->height(), vl_img->depth()));


  // note img has been converted unconditionally to IT_UNSIGNED_SHORT above!
  fx->shader()->gocTextureSampler(0)->setTexture(new vl::Texture(vl_img.get(), vl::TF_LUMINANCE16, false, false));
  fx->shader()->gocUniform("volume_texunit")->setUniformI(0);

  // generate a simple colored transfer function
  ref<vl::Image>  trfunc = vl::makeColorSpectrum(1024, vl::blue, vl::royalblue, vl::green, vl::yellow, vl::crimson);
  // installs the transfer function as texture #1
  fx->shader()->gocTextureSampler(1)->setTexture(new vl::Texture(trfunc.get()));
  fx->shader()->gocUniform("trfunc_texunit")->setUniformI(1);
/*
  ref<Image> gradient;
  // note that this can take a while...
  gradient = vl::genGradientNormals( vl_img.get() );
  fx->shader()->gocUniform( "precomputed_gradient" )->setUniformI( 1);
  fx->shader()->gocTextureSampler( 2 )->setTexture( new Texture( gradient.get(), TF_RGBA, false, false ) );
  fx->shader()->gocUniform( "gradient_texunit" )->setUniformI( 2 );
*/
  fx->shader()->gocUniform("precomputed_gradient")->setUniformI(0);
  // used to compute on the fly the normals based on the volume's gradient
  fx->shader()->gocUniform("gradient_delta")->setUniform(vl::fvec3(0.5f / vl_img->width(), 0.5f / vl_img->height(), 0.5f / vl_img->depth()));

  fx->shader()->gocUniform( "sample_step" )->setUniformF(1.0f / 512.0f);

  vtkLinearTransform * nodeVtkTr = mitkImg->GetGeometry()->GetVtkTransform();
  vtkSmartPointer<vtkMatrix4x4> geometryTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  nodeVtkTr->GetMatrix(geometryTransformMatrix);

  float vals[16];
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      double val = geometryTransformMatrix->GetElement(i, j);
      vals[i*4+j] = val;
    }
  }
  vl::mat4 mat(vals);
  tr->setLocalMatrix(mat);

  // refresh window
  //openglContext()->update();

  return imageActor;
}

//-----------------------------------------------------------------------------

vl::EImageType VLQtWidget::MapITKPixelTypeToVL(int itkComponentType)
{
  static const vl::EImageType typeMap[] =
  {
    vl::IT_IMPLICIT_TYPE,   // itk::ImageIOBase::UNKNOWNCOMPONENTTYPE = 0
    vl::IT_UNSIGNED_BYTE,   // itk::ImageIOBase::UCHAR = 1
    vl::IT_BYTE,            // itk::ImageIOBase::CHAR = 2
    vl::IT_UNSIGNED_SHORT,  // itk::ImageIOBase::USHORT = 3
    vl::IT_SHORT,           // itk::ImageIOBase::SHORT = 4
    vl::IT_UNSIGNED_INT,    // itk::ImageIOBase::UINT = 5
    vl::IT_INT,             // itk::ImageIOBase::INT = 6
    vl::IT_IMPLICIT_TYPE,   // itk::ImageIOBase::ULONG = 7
    vl::IT_IMPLICIT_TYPE,   // itk::ImageIOBase::LONG = 8
    vl::IT_FLOAT,           // itk::ImageIOBase::FLOAT = 9
    vl::IT_IMPLICIT_TYPE    // itk::ImageIOBase::DOUBLE = 10
  };

  return typeMap[itkComponentType];
}

//-----------------------------------------------------------------------------

vl::EImageFormat VLQtWidget::MapComponentsToVLColourFormat(int components)
{
  // this assumes the image data is a normal colour image, not encoding pointers or indices, or similar stuff.

  switch (components)
  {
    default:
    case 1:
      return vl::IF_LUMINANCE;
    case 2:
      return vl::IF_RG;
    case 3:
      return vl::IF_RGB;
    case 4:
      return vl::IF_RGBA;
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::SetBackgroundColour(float r, float g, float b)
{
  m_VividRendering->camera()->viewport()->setClearColor(vl::fvec4(r, g, b, 1));
  update();
}

//-----------------------------------------------------------------------------

void VLQtWidget::initializeGL()
{
  VIVID_CHECK( QGLContext::currentContext() == QGLWidget::context() );

  vl::OpenGLContext::initGLContext();

  // Interface VL with Qt's resource system to load GLSL shaders.
  vl::defFileSystem()->directories().clear();
  vl::defFileSystem()->directories().push_back( new vl::QtDirectory( ":/VL/" ) );


#if 0
  // Mic: this seems to be failing for me.
  // use the device that is running our opengl context as the compute-device
  // for sorting triangles in the correct order.
  if (m_OclService)
  {
    // Force tests to run on the first GPU with shared context
    m_OclService->SpecifyPlatformAndDevice(0, 0, true);
    // Calling this to make sure that the context is created right at startup
    cl_context clContext = m_OclService->GetContext();
  }
#endif

#ifdef _MSC_VER
  // NvAPI_OGL_ExpertModeSet(NVAPI_OGLEXPERT_DETAIL_ALL, NVAPI_OGLEXPERT_DETAIL_BASIC_INFO, NVAPI_OGLEXPERT_OUTPUT_TO_ALL, 0);
#endif

  // Create our VividRendering!
  m_VividRendering = new vl::VividRendering( vl::OpenGLContext::framebuffer() );
  m_VividRendering->setRenderingMode( vl::VividRendering::FrontToBackDepthPeeling ); /* (default) */
  m_VividRendering->setCullingEnabled( true );
  // This creates some flickering on the skin for some reason
  m_VividRendering->setNearFarClippingPlanesOptimized( false );

  // VividRendering nicely prepares for us all the structures we need to use ;)
  m_VividRenderer = m_VividRendering->vividRenderer();
  m_VividVolume = m_VividRendering->vividVolume();
  m_SceneManager = m_VividRendering->sceneManager();
  m_Camera = m_VividRendering->calibratedCamera();

  // Initialize the trackball manipulator
  m_Trackball = new VLTrackballManipulator;
  m_Trackball->setEnabled( true );
  m_Trackball->setCamera( m_Camera.get() );
  m_Trackball->setTransform( NULL );
  m_Trackball->setPivot( vl::vec3(0,0,0) );
  vl::OpenGLContext::addEventListener( m_Trackball.get() );
  // Schedule reset of the camera based on the scene content
  ScheduleTrackballAdjustView();

  // This is only used by the CUDA stuff
  CreateAndUpdateFBOSizes( QGLWidget::width(), QGLWidget::height() );

  vl::OpenGLContext::dispatchInitEvent();

#if 0
  // Point cloud data test
  mitk::DataNode::Pointer n = mitk::DataNode::New();
  mitk::PCLData::Pointer  p = niftk::PCLData::New();
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr  c(new pcl::PointCloud<pcl::PointXYZRGB>);
  for (int i = 0; i < 100; ++i)
  {
    pcl::PointXYZRGB  q(std::rand() % 255, std::rand() % 255, std::rand() % 255);
    q.x = std::rand() % 255;
    q.y = std::rand() % 255;
    q.z = std::rand() % 255;
    c->push_back(q);
  }
  p->SetCloud(c);
  n->SetData(p);

  m_DataStorage->Add(n);
#endif

#if 0
  // Update the rendering at 5 fps so video card doesn't sleep.
  // Avoid "sticky" effect when rotating scene with mouse.
  disconnect(&m_BackgroundUpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
  connect(&m_BackgroundUpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
  m_BackgroundUpdateTimer.setSingleShot(false);
  m_BackgroundUpdateTimer.start(1000 / 5);
#endif
}

//-----------------------------------------------------------------------------

void VLQtWidget::resizeGL( int w, int h )
{
   VIVID_CHECK( QGLContext::currentContext() == QGLWidget::context() );

  // dont do anything if window is zero size.
  // it's an opengl error to have a viewport like that!
  if ( w <= 0 || h <= 0 ) {
    return;
  }

  m_VividRendering->camera()->viewport()->set( 0, 0, w, h );
  m_VividRendering->camera()->setProjectionPerspective();

  CreateAndUpdateFBOSizes( w, h );

  // MIC FIXME: update calibrated camera setup
  UpdateViewportAndCameraAfterResize();

  vl::OpenGLContext::dispatchResizeEvent( w, h );
}

//-----------------------------------------------------------------------------

void VLQtWidget::paintGL()
{
  VIVID_CHECK( QGLContext::currentContext() == QGLWidget::context() );

  RenderScene();

  vl::OpenGLContext::dispatchRunEvent();
}

//-----------------------------------------------------------------------------

void VLQtWidget::CreateAndUpdateFBOSizes( int width, int height )
{
  makeCurrent();

#ifdef _USE_CUDA
  // sanitise dimensions. depending on how windows are resized we can get zero here.
  // but that breaks on the ogl side.
  width  = std::max(1, width);
  height = std::max(1, height);

  ref<vl::FramebufferObject> opaqueFBO = vl::OpenGLContext::createFramebufferObject(width, height);
  opaqueFBO->setObjectName("opaqueFBO");
  opaqueFBO->addDepthAttachment(new vl::FBODepthBufferAttachment(vl::DBF_DEPTH_COMPONENT24));
  opaqueFBO->addColorAttachment(vl::AP_COLOR_ATTACHMENT0, new vl::FBOColorBufferAttachment(vl::CBF_RGBA));   // this is a renderbuffer
  opaqueFBO->setDrawBuffer(vl::RDB_COLOR_ATTACHMENT0);

  if (m_CUDAInteropPimpl)
  {
    delete m_CUDAInteropPimpl->m_FBOAdaptor;
    m_CUDAInteropPimpl->m_FBOAdaptor = new VLFramebufferAdaptor(opaqueFBO.get());
  }
#endif
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateViewportAndCameraAfterResize()
{
  // some sane defaults
  // m_Camera->viewport()->set( 0, 0, QWidget::width(), QWidget::height() );
  // m_BackgroundCamera->viewport()->set(0, 0, QWidget::width(), QWidget::height());

  if ( m_BackgroundNode.IsNotNull() )
  {
    //NodeActorMapType::iterator ni = m_NodeActorMap.find(m_BackgroundNode);
    //if (ni == m_NodeActorMap.end())
    //{
    //  // actor not ready yet, try again later.
    //  // this is getting messy... but stuffing our widget here into an editor causes various methods
    //  // to be called at the wrong time.
    //  QMetaObject::invokeMethod(this, "UpdateViewportAndCameraAfterResize", Qt::QueuedConnection);
    //}
    //else
    //{
      // ref<vl::Actor> backgroundactor = ni->second;

      // this is based on my old araknes video-ar app.
      // FIXME: aspect ratio?
      float   width_scale  = (float) QWidget::width()  / (float) m_BackgroundWidth;
      float   height_scale = (float) QWidget::height() / (float) m_BackgroundHeight;
      int     vpw = QWidget::width();
      int     vph = QWidget::height();
      if (width_scale < height_scale)
        vph = (int) ((float) m_BackgroundHeight * width_scale);
      else
        vpw = (int) ((float) m_BackgroundWidth * height_scale);

      int   vpx = QWidget::width()  / 2 - vpw / 2;
      int   vpy = QWidget::height() / 2 - vph / 2;

      // m_BackgroundCamera->viewport()->set(vpx, vpy, vpw, vph);
      // the main-scene-camera should conform to this viewport too!
      // otherwise geometry would never line up with the background (for overlays, etc).
      m_Camera->viewport()->set(vpx, vpy, vpw, vph);
    //}
  }
  // this default perspective depends on the viewport!
  m_Camera->setProjectionPerspective();

  UpdateCameraParameters();
}

void VLQtWidget::UpdateScene() {
  // Make sure the system is initialized
  VIVID_CHECK( m_VividRendering.get() );
  VIVID_CHECK( QGLContext::currentContext() == QGLWidget::context() );

  if ( m_SceneManager->tree()->actors()->empty() ) {
    InitSceneFromDataStorage();
  } else {
    // Execute scheduled removals
    for ( std::set<mitk::DataNode::ConstPointer>::const_iterator it = m_NodesToRemove.begin(); it != m_NodesToRemove.end(); ++it)
    {
      RemoveDataNode(*it);
    }
    m_NodesToRemove.clear();

    // Execute scheduled additions
    for ( std::set<mitk::DataNode::ConstPointer>::const_iterator it = m_NodesToAdd.begin(); it != m_NodesToAdd.end(); ++it)
    {
      AddDataNode(*it);
    }
    m_NodesToAdd.clear();

    // Execute scheduled updates
    for ( std::set<mitk::DataNode::ConstPointer>::const_iterator it = m_NodesToUpdate.begin(); it != m_NodesToUpdate.end(); ++it)
    {
      UpdateDataNode(*it);
    }
    m_NodesToUpdate.clear();
  }

  // Reset trackball view on demand

  if ( m_ScheduleTrackballAdjustView ) {
    m_Trackball->adjustView( m_VividRendering.get(), vl::vec3(0,0,1), vl::vec3(0,1,0), 1.0f );
    m_ScheduleTrackballAdjustView = false;
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::RenderScene()
{
  makeCurrent();

  UpdateScene();

  // Set frame time for all the rendering
  vl::real now_time = vl::Time::currentTime();
  m_VividRendering->setFrameClock( now_time );

  // Execute rendering
  m_VividRendering->render();

  // Show rendering
  if ( vl::OpenGLContext::hasDoubleBuffer() ) {
    swapBuffers();
  }

  VL_CHECK_OGL();
}

//-----------------------------------------------------------------------------

void VLQtWidget::ClearScene()
{
  makeCurrent();

  if ( m_SceneManager )
  {
    if ( m_SceneManager->tree() ) {
      m_SceneManager->tree()->actors()->clear();
    }
  }

  m_CameraNode = 0;
  m_BackgroundNode = 0;
  m_NodeActorMap.clear();
  m_NodesToUpdate.clear();
  m_NodesToAdd.clear();
  m_NodesToRemove.clear();
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateThresholdVal( int isoVal )
{
  float iso = isoVal / 10000.0f;
  iso = vl::clamp( iso, 0.0f, 1.0f );
  m_VividRendering->vividVolume()->setIsoValue( iso );
}

//-----------------------------------------------------------------------------

bool VLQtWidget::SetCameraTrackingNode(const mitk::DataNode::ConstPointer& node)
{
  VIVID_CHECK( m_Trackball );

  // Whenever we set the camera node to NULL we recenter the scene using the trackball

  m_CameraNode = node;

  if (m_CameraNode.IsNull())
  {
    m_Trackball->setEnabled( true );
    ScheduleTrackballAdjustView( true );
  } else {
    m_Trackball->setEnabled( false );
    ScheduleTrackballAdjustView( false );
    UpdateCameraParameters();
  }

  update();

  return true;
}

//-----------------------------------------------------------------------------

vl::mat4 VLQtWidget::GetVLMatrixFromData(const mitk::BaseData::ConstPointer& data)
{
  vl::mat4  mat;
  // intentionally not setIdentity()
  mat.setNull();

  if (data.IsNotNull())
  {
    mitk::BaseGeometry::Pointer   geom = data->GetGeometry();
    if (geom.IsNotNull())
    {
      if (geom->GetVtkTransform() != 0)
      {
        vtkSmartPointer<vtkMatrix4x4> vtkmat = vtkSmartPointer<vtkMatrix4x4>::New();
        geom->GetVtkTransform()->GetMatrix(vtkmat);
        if (vtkmat.GetPointer() != 0)
        {
          for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 4; j++)
            {
              double val = vtkmat->GetElement(i, j);
              mat.e(i, j) = val;
            }
          }
        }
      }
    }
  }

  return mat;
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateTransformFromData(vl::Transform* txf, const mitk::BaseData::ConstPointer& data)
{
  vl::mat4  mat = GetVLMatrixFromData(data);

  if (!mat.isNull())
  {
    txf->setLocalMatrix(mat);
    txf->computeWorldMatrix();
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateActorTransformFromNode(vl::Actor* actor, const mitk::DataNode::ConstPointer& node)
{
  if (node.IsNotNull())
  {
    mitk::BaseData::Pointer data = node->GetData();
    if (data.IsNotNull())
    {
      mitk::BaseGeometry::Pointer geom = data->GetGeometry();
      if (geom.IsNotNull())
      {
        ref<VLUserData> userdata = GetUserData(actor);
        if (geom->GetMTime() > userdata->m_TransformModifiedTime)
        {
          UpdateTransformFromData(actor->transform(), data.GetPointer());
          userdata->m_TransformModifiedTime = geom->GetMTime();
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------

VLUserData* VLQtWidget::GetUserData(vl::Actor* actor)
{
  VIVID_CHECK( actor );
  ref<VLUserData> userdata = actor->userData()->as<VLUserData>();
  if ( ! userdata )
  {
    userdata = new VLUserData;
    actor->setUserData( userdata.get() );
  }

  return userdata.get();
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateTransformFromNode(vl::Transform* txf, const mitk::DataNode::ConstPointer& node)
{
  if (node.IsNotNull())
  {
    UpdateTransformFromData(txf, node->GetData());
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateCameraParameters()
{
  // calibration parameters come from the background node.
  // so no background, no camera parameters.
  if (m_BackgroundNode.IsNotNull())
  {
#ifdef BUILD_IGI
    mitk::BaseProperty::Pointer cambp = m_BackgroundNode->GetProperty(niftk::Undistortion::s_CameraCalibrationPropertyName);
    if (cambp.IsNotNull())
    {
      mitk::CameraIntrinsicsProperty::Pointer cam = dynamic_cast<mitk::CameraIntrinsicsProperty*>(cambp.GetPointer());
      if (cam.IsNotNull())
      {
        mitk::CameraIntrinsics::Pointer nodeIntrinsic = cam->GetValue();

        if (nodeIntrinsic.IsNotNull())
        {
          // based on niftkCore/Rendering/vtkOpenGLMatrixDrivenCamera
          float znear = 1;
          float zfar  = 10000;
          float pixelaspectratio = 1;   // FIXME: depends on background image

          vl::mat4  proj;
          proj.setNull();
          proj.e(0, 0) =  2 * nodeIntrinsic->GetFocalLengthX() / (float) m_BackgroundWidth;
          //proj.e(0, 1) = -2 * 0 / m_ImageWidthInPixels;
          proj.e(0, 2) = ((float) m_BackgroundWidth - 2 * nodeIntrinsic->GetPrincipalPointX()) / (float) m_BackgroundWidth;
          proj.e(1, 1) = 2 * (nodeIntrinsic->GetFocalLengthY() / pixelaspectratio) / ((float) m_BackgroundHeight / pixelaspectratio);
          proj.e(1, 2) = (-((float) m_BackgroundHeight / pixelaspectratio) + 2 * (nodeIntrinsic->GetPrincipalPointY() / pixelaspectratio)) / ((float) m_BackgroundHeight / pixelaspectratio);
          proj.e(2, 2) = (-zfar - znear) / (zfar - znear);
          proj.e(2, 3) = -2 * zfar * znear / (zfar - znear);
          proj.e(3, 2) = -1;

          m_Camera->setProjectionMatrix(proj.transpose(), vl::PMT_UserProjection);
        }
      }
    }
#endif
  }

  if (m_CameraNode.IsNotNull())
  {
    vl::mat4 mat = GetVLMatrixFromData(m_CameraNode->GetData());
    if ( ! mat.isNull() ) {
      // beware: there is also a view-matrix! the inverse of modelling-matrix.
      m_Camera->setModelingMatrix(mat);
    }
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::PrepareBackgroundActor(const mitk::Image* img, const mitk::BaseGeometry* geom, const mitk::DataNode::ConstPointer node)
{
  makeCurrent();

  // nasty
  mitk::Image::Pointer imgp(const_cast<mitk::Image*>(img));
  ref<vl::Actor> actor = Add2DImageActor(imgp);


  // essentially copied from vl::makeGrid()
  ref<vl::Geometry>         vlquad = new vl::Geometry;

  ref<vl::ArrayFloat3> vert3 = new vl::ArrayFloat3;
  vert3->resize(4);
  vlquad->setVertexArray(vert3.get());

  ref<vl::ArrayFloat2> text2 = new vl::ArrayFloat2;
  text2->resize(4);
  vlquad->setTexCoordArray(0, text2.get());

  //  0---3
  //  |   |
  //  1---2
  vert3->at(0).x() = -1; vert3->at(0).y() =  1; vert3->at(0).z() = 0;  text2->at(0).s() = 0; text2->at(0).t() = 0;
  vert3->at(1).x() = -1; vert3->at(1).y() = -1; vert3->at(1).z() = 0;  text2->at(1).s() = 0; text2->at(1).t() = 1;
  vert3->at(2).x() =  1; vert3->at(2).y() = -1; vert3->at(2).z() = 0;  text2->at(2).s() = 1; text2->at(2).t() = 1;
  vert3->at(3).x() =  1; vert3->at(3).y() =  1; vert3->at(3).z() = 0;  text2->at(3).s() = 1; text2->at(3).t() = 0;


  ref<vl::DrawElementsUInt> polys = new vl::DrawElementsUInt(vl::PT_QUADS);
  polys->indexBuffer()->resize(4);
  polys->indexBuffer()->at(0) = 0;
  polys->indexBuffer()->at(1) = 1;
  polys->indexBuffer()->at(2) = 2;
  polys->indexBuffer()->at(3) = 3;
  vlquad->drawCalls().push_back(polys.get());

  // replace original quad with ours.
  actor->setLod(0, vlquad.get());
  actor->effect()->shader()->disable(vl::EN_LIGHTING);

  std::string   objName = actor->objectName() + "_background";
  actor->setObjectName(objName.c_str());

  m_NodeActorMap[node] = actor;
}

//-----------------------------------------------------------------------------

bool VLQtWidget::SetBackgroundNode(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  // clear up after previous background node.
  if (m_BackgroundNode.IsNotNull())
  {
    const mitk::DataNode::ConstPointer    oldbackgroundnode = m_BackgroundNode;
    m_BackgroundNode = 0;
    RemoveDataNode(oldbackgroundnode);
    // add back as normal node.
    AddDataNode(oldbackgroundnode);
  }

  // default "no background" value.
  m_BackgroundWidth  = 0;
  m_BackgroundHeight = 0;

  bool    result = false;
  mitk::BaseData::Pointer   basedata;
  if (node.IsNotNull())
    basedata = node->GetData();
  if (basedata.IsNotNull())
  {
    // clear up whatever we had cached for the new background node.
    // it's very likely that it was a normal node before.
    RemoveDataNode(node);

    mitk::Image::Pointer imgdata = dynamic_cast<mitk::Image*>(basedata.GetPointer());
    if (imgdata.IsNotNull())
    {
#ifdef _USE_CUDA
      niftk::CUDAImageProperty::Pointer    cudaimgprop = dynamic_cast<niftk::CUDAImageProperty*>(imgdata->GetProperty("CUDAImageProperty").GetPointer());
      if (cudaimgprop.IsNotNull())
      {
        niftk::LightweightCUDAImage    lwci = cudaimgprop->Get();

        // does the size of cuda-image have to match the mitk-image where it's attached to?
        // i think it does: it is supposed to be the same data living in cuda.
        VIVID_CHECK(lwci.GetWidth()  == imgdata->GetDimension(0));
        VIVID_CHECK(lwci.GetHeight() == imgdata->GetDimension(1));

        PrepareBackgroundActor(&lwci, imgdata->GetGeometry(), node);
        result = true;
      }
      else
#endif
      {
        PrepareBackgroundActor(imgdata.GetPointer(), imgdata->GetGeometry(), node);
        result = true;
      }

      m_BackgroundWidth  = imgdata->GetDimension(0);
      m_BackgroundHeight = imgdata->GetDimension(1);
    }
    else
    {
#ifdef _USE_CUDA
      niftk::CUDAImage::Pointer    cudaimgdata = dynamic_cast<niftk::CUDAImage*>(basedata.GetPointer());
      if (cudaimgdata.IsNotNull())
      {
        niftk::LightweightCUDAImage    lwci = cudaimgdata->GetLightweightCUDAImage();
        PrepareBackgroundActor(&lwci, cudaimgdata->GetGeometry(), node);
        result = true;

        m_BackgroundWidth  = lwci.GetWidth();
        m_BackgroundHeight = lwci.GetHeight();
      }
      // no else here
#endif
    }

    // UpdateDataNode() depends on m_BackgroundNode.
    m_BackgroundNode = node;
    UpdateDataNode(node);
  }

  UpdateViewportAndCameraAfterResize();

  // now that the camera may have changed, fit-view-to-scene again.
  //if (m_CameraNode.IsNull())
  //{
  //  m_Trackball->setEnabled( true );
  //  m_Trackball->adjustView(m_SceneManager.get(), vl::vec3(0, 0, 1), vl::vec3(0, 1, 0), 1.0f);
  //}

  return result;
}

//-----------------------------------------------------------------------------

vl::Actor* VLQtWidget::GetNodeActor(const mitk::DataNode::ConstPointer& node)
{
  NodeActorMapType::iterator it = m_NodeActorMap.find(node);
  return it == m_NodeActorMap.end() ? NULL : it->second.get();
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateTextureFromImage(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  if ( node.IsNull() ) {
    return;
  }

  mitk::Image::Pointer mitk_img = dynamic_cast<mitk::Image*>(node->GetData());
  if ( mitk_img.IsNull() ) {
    return;
  }

  vl::Actor* actor = GetNodeActor(node);
  if ( actor )
  {
    VIVID_CHECK(actor->effect());
    VIVID_CHECK(actor->effect()->shader());

    VLUserData* userdata = GetUserData(actor);
    if (mitk_img->GetVtkImageData()->GetMTime() > userdata->m_ImageModifiedTime)
    {
      ref<vl::Texture> tex = actor->effect()->shader()->gocTextureSampler(0)->texture();
      if (tex.get() != 0)
      {
        unsigned int*       dims    = mitk_img->GetDimensions();    // we do not own dims!
        mitk::PixelType     mitk_pixel_type = mitk_img->GetPixelType();
        vl::EImageType      type    = MapITKPixelTypeToVL(mitk_pixel_type.GetComponentType());
        vl::EImageFormat    format  = MapComponentsToVLColourFormat(mitk_pixel_type.GetNumberOfComponents());

        ref<vl::Image>    vlimg = new vl::Image(dims[0], dims[1], 0, 1, format, type);
        // sanity check
        unsigned int  size = (dims[0] * dims[1] * dims[2]) * mitk_pixel_type.GetSize();
        VIVID_CHECK(vlimg->requiredMemory() == size);

        try
        {
          mitk::ImageReadAccessor   readAccess(mitk_img);
          const void*               cPointer = readAccess.GetData();
          std::memcpy(vlimg->pixels(), cPointer, vlimg->requiredMemory());
        }
        catch (...)
        {
          // FIXME: error handling?
          MITK_ERROR << "Did not get pixel read access to 2D image.";
        }

        tex->setMipLevel(0, vlimg.get(), false);

        userdata->m_ImageModifiedTime = mitk_img->GetVtkImageData()->GetMTime();
      }
    }
  }
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddCoordinateAxisActor(const mitk::CoordinateAxesData::Pointer& coord)
{
  makeCurrent();

  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), coord.GetPointer());

  ref<vl::ArrayFloat3> verts  = new vl::ArrayFloat3;
  ref<vl::ArrayFloat4> colors = new vl::ArrayFloat4;
  verts->resize(6);
  colors->resize(6);
  
  // Axis length
  float S = 100;

  // X Axis
  verts ->at(0) = vec3(0, 0, 0);
  verts ->at(1) = vec3(S, 0, 0);
  colors->at(0) = vl::red;
  colors->at(1) = vl::red;
  // Y Axis
  verts ->at(2) = vec3(0, 0, 0);
  verts ->at(3) = vec3(0, S, 0);
  colors->at(2) = vl::green;
  colors->at(3) = vl::green;
  // Z Axis
  verts ->at(4) = vec3(0, 0, 0);
  verts ->at(5) = vec3(0, 0, S);
  colors->at(4) = vl::blue;
  colors->at(5) = vl::blue;

  ref<vl::Geometry> geom = new vl::Geometry;
  geom->drawCalls().push_back( new vl::DrawArrays( vl::PT_LINES, 0, 6 ) );
  geom->setVertexArray(verts.get());
  geom->setColorArray(colors.get());

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();
  fx->shader()->getLineWidth()->set( 2 );
  // Use color array instead of lighting
  fx->shader()->gocUniform( "vl_Vivid.enableLighting" )->setUniformI( 0 );

  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(geom.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );

  return actor;
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddPointCloudActor(niftk::PCLData* pcl)
{
  makeCurrent();

#ifdef _USE_PCL
  pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr cloud = pcl->GetCloud();

  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), pcl);

  ref<vl::ArrayFloat3> vl_verts = new vl::ArrayFloat3;
  ref<vl::ArrayFloat4> vl_colors = new vl::ArrayFloat4;
  vl_verts->resize(cloud->size());
  vl_colors->resize(cloud->size());
  // We could interleave the color and vert array but would we trust the VTK layout?
  int j = 0;
  for (pcl::PointCloud<pcl::PointXYZRGB>::const_iterator i = cloud->begin(); i != cloud->end(); ++i, ++j)
  {
    const pcl::PointXYZRGB& p = *i;

    vl_verts->at(j).x() = p.x;
    vl_verts->at(j).y() = p.y;
    vl_verts->at(j).z() = p.z;

    vl_colors->at(j).r() = (float)p.r / 255.0f;
    vl_colors->at(j).g() = (float)p.g / 255.0f;
    vl_colors->at(j).b() = (float)p.b / 255.0f;
    vl_colors->at(j).a() = 1;
  }

  ref<vl::Geometry> geom = new vl::Geometry;
  ref<vl::DrawArrays> draw_arrays = new vl::DrawArrays(vl::PT_POINTS, 0, vl_verts->size());
  geom->drawCalls().push_back(draw_arrays.get());
  geom->setVertexArray(vl_verts.get());
  geom->setColorArray(vl_colors.get());

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();

  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(geom.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );

  return actor;
#else
  throw std::runtime_error("No PCL-support enabled at compile time!");
#endif
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddPointsetActor(const mitk::PointSet::Pointer& mitkPS)
{
  makeCurrent();

  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), mitkPS.GetPointer());

  ref<vl::ArrayFloat3> verts = new vl::ArrayFloat3;
  verts->resize(mitkPS->GetSize());
  int j = 0;
  for (mitk::PointSet::PointsConstIterator i = mitkPS->Begin(); i != mitkPS->End(); ++i, ++j)
  {
    mitk::PointSet::PointType p = i->Value();
    verts->at(j).x() = p[0];
    verts->at(j).y() = p[1];
    verts->at(j).z() = p[2];
  }

  ref<vl::Geometry> geom = new vl::Geometry;
  ref<vl::DrawArrays> draw_arrays = new vl::DrawArrays(vl::PT_POINTS, 0, verts->size());
  geom->drawCalls().push_back(draw_arrays.get());
  geom->setVertexArray(verts.get());

  ref<vl::Effect> fx = vl::VividRendering::makeVividEffect();

  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(geom.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );

  return actor;
}

//-----------------------------------------------------------------------------

ref<vl::Geometry> VLQtWidget::CreateGeometryFor2DImage(int width, int height)
{
  ref<vl::Geometry>    geom = new vl::Geometry;
  ref<vl::ArrayFloat3> vert  = new vl::ArrayFloat3;
  vert->resize(4);
  geom->setVertexArray( vert.get() );

  ref<vl::ArrayFloat2> tex_coord = new vl::ArrayFloat2;
  tex_coord->resize(4);
  geom->setTexCoordArray(0, tex_coord.get());

  //  1---2 image-top
  //  |   |
  //  0---3 image-bottom

  vert->at(0).x() = 0;     vert->at(0).y() = 0;      vert->at(0).z() = 0; tex_coord->at(0).s() = 0; tex_coord->at(0).t() = 1;
  vert->at(1).x() = 0;     vert->at(1).y() = height; vert->at(1).z() = 0; tex_coord->at(1).s() = 0; tex_coord->at(1).t() = 0;
  vert->at(2).x() = width; vert->at(2).y() = height; vert->at(2).z() = 0; tex_coord->at(2).s() = 1; tex_coord->at(2).t() = 0;
  vert->at(3).x() = width; vert->at(3).y() = 0;      vert->at(3).z() = 0; tex_coord->at(3).s() = 1; tex_coord->at(3).t() = 1;

  ref<vl::DrawArrays> polys = new vl::DrawArrays(vl::PT_QUADS, 0, 4);
  geom->drawCalls().push_back( polys.get() );

  return geom;
}

//-----------------------------------------------------------------------------

void VLQtWidget::setContinuousUpdate(bool continuous)
{
  vl::OpenGLContext::setContinuousUpdate(continuous);

  if (continuous)
  {
    disconnect(&m_UpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
    connect(&m_UpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
    m_UpdateTimer.setSingleShot(false);
    m_UpdateTimer.start(m_Refresh);
  }
  else
  {
    disconnect(&m_UpdateTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
    m_UpdateTimer.stop();
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::setWindowTitle(const vl::String& title)
{
  QGLWidget::setWindowTitle( QString::fromStdString(title.toStdString()) );
}

//-----------------------------------------------------------------------------

bool VLQtWidget::setFullscreen(bool fullscreen)
{
  // fullscreen not allowed (yet)!
  fullscreen = false;

  mFullscreen = fullscreen;
  if (fullscreen)
    QGLWidget::setWindowState(QGLWidget::windowState() | Qt::WindowFullScreen);
  else
    QGLWidget::setWindowState(QGLWidget::windowState() & (~Qt::WindowFullScreen));
  return true;
}


//-----------------------------------------------------------------------------
void VLQtWidget::show()
{
  QGLWidget::show();
}


//-----------------------------------------------------------------------------
void VLQtWidget::hide()
{
  QGLWidget::hide();
}


//-----------------------------------------------------------------------------
void VLQtWidget::setPosition(int x, int y)
{
  QGLWidget::move(x,y);
}


//-----------------------------------------------------------------------------
vl::ivec2 VLQtWidget::position() const
{
  return vl::ivec2(QGLWidget::pos().x(), QGLWidget::pos().y());
}


//-----------------------------------------------------------------------------
void VLQtWidget::update()
{
  // schedules a repaint, will eventually call into paintGL()
  QGLWidget::update();
}


//-----------------------------------------------------------------------------
void VLQtWidget::setSize(int w, int h)
{
  // this already excludes the window's frame so it's ok for Visualization Library standards
  QGLWidget::resize(w,h);
}


//-----------------------------------------------------------------------------
vl::ivec2 VLQtWidget::size() const
{
  // this already excludes the window's frame so it's ok for Visualization Library standards
  return vl::ivec2(QGLWidget::size().width(), QGLWidget::size().height());
}


//-----------------------------------------------------------------------------
void VLQtWidget::swapBuffers()
{
  makeCurrent();

  QGLWidget::swapBuffers();

#ifdef _USE_CUDA
  if (m_CUDAInteropPimpl)
  {
    cudaError_t          err         = cudaSuccess;
    niftk::CUDAManager*  cudamanager = niftk::CUDAManager::GetInstance();
    cudaStream_t         mystream    = cudamanager->GetStream(m_CUDAInteropPimpl->m_NodeName);
    niftk::WriteAccessor outputWA    = cudamanager->RequestOutputImage(QWidget::width(), QWidget::height(), 4);
    cudaArray_t          fboarr      = m_CUDAInteropPimpl->m_FBOAdaptor->Map(mystream);

    // side note: cuda-arrays are always measured in bytes, never in pixels.
    err = cudaMemcpy2DFromArrayAsync(outputWA.m_DevicePointer, outputWA.m_BytePitch, fboarr, 0, 0, outputWA.m_PixelWidth * 4, outputWA.m_PixelHeight, cudaMemcpyDeviceToDevice, mystream);
    // not sure what to do if it fails. do not throw an exception, that's for sure.
    if (err != cudaSuccess)
    {
      VIVID_CHECK(false);
    }

    // the opengl-interop side is done, renderer can continue from now on.
    m_CUDAInteropPimpl->m_FBOAdaptor->Unmap(mystream);

    // need to flip the image! ogl is left-bottom, but everywhere else is left-top origin!
    niftk::WriteAccessor  flippedWA   = cudamanager->RequestOutputImage(outputWA.m_PixelWidth, outputWA.m_PixelHeight, 4);
    // FIXME: instead of explicitly flipping we could bind the fboarr to a texture, and do a single write out.
    niftk::FlipImageLauncher(outputWA, flippedWA, mystream);

    niftk::LightweightCUDAImage lwciFlipped = cudamanager->Finalise(flippedWA, mystream);
    // Finalise() needs to come before Autorelease(), for performance reasons.
    cudamanager->Autorelease(outputWA, mystream);

    bool    isNewNode = false;
    mitk::DataNode::Pointer node = m_CUDAInteropPimpl->m_DataStorage->GetNamedNode(m_CUDAInteropPimpl->m_NodeName);
    if (node.IsNull())
    {
      isNewNode = true;
      node = mitk::DataNode::New();
      node->SetName(m_CUDAInteropPimpl->m_NodeName);
      node->SetVisibility(false);
      //node->SetBoolProperty("helper object", true);
    }
    niftk::CUDAImage::Pointer  img = dynamic_cast<niftk::CUDAImage*>(node->GetData());
    if (img.IsNull())
      img = niftk::CUDAImage::New();
    img->SetLightweightCUDAImage(lwciFlipped);
    node->SetData(img);
    if (isNewNode)
      m_CUDAInteropPimpl->m_DataStorage->Add(node);
    else
      node->Modified();
  }
#endif
}

//-----------------------------------------------------------------------------

void VLQtWidget::makeCurrent()
{
  QGLWidget::makeCurrent();
  // sanity check
  VIVID_CHECK( QGLContext::currentContext() == QGLWidget::context() );
}

//-----------------------------------------------------------------------------

void VLQtWidget::setMousePosition(int x, int y)
{
  QCursor::setPos(mapToGlobal(QPoint(x,y)));
}

//-----------------------------------------------------------------------------

void VLQtWidget::setMouseVisible(bool visible)
{
  vl::OpenGLContext::setMouseVisible(visible);

  if (visible)
    QGLWidget::setCursor(Qt::ArrowCursor);
  else
    QGLWidget::setCursor(Qt::BlankCursor);
}

//-----------------------------------------------------------------------------

void VLQtWidget::getFocus()
{
  QGLWidget::setFocus(Qt::OtherFocusReason);
}

//-----------------------------------------------------------------------------

void VLQtWidget::setRefreshRate(int msec)
{
  m_Refresh = msec;
  m_UpdateTimer.setInterval(m_Refresh);
}

//-----------------------------------------------------------------------------

int VLQtWidget::refreshRate()
{
  return m_Refresh;
}

#if 0
//-----------------------------------------------------------------------------
void VLQtWidget::dragEnterEvent(QDragEnterEvent *ev)
{
  if (ev->mimeData()->hasUrls())
    ev->acceptProposedAction();
}


//-----------------------------------------------------------------------------
void VLQtWidget::dropEvent(QDropEvent* ev)
{
  if ( ev->mimeData()->hasUrls() )
  {
    std::vector<vl::String> files;
    QList<QUrl> list = ev->mimeData()->urls();
    for(int i=0; i<list.size(); ++i)
    {
      if (list[i].path().isEmpty())
        continue;
      #ifdef WIN32
        if (list[i].path()[0] == '/')
          files.push_back( list[i].path().toStdString().c_str()+1 );
        else
          files.push_back( list[i].path().toStdString().c_str() );
      #else
        files.push_back( list[i].path().toStdString().c_str() );
      #endif
    }
    dispatchFileDroppedEvent(files);
  }
}
#endif

//-----------------------------------------------------------------------------

void VLQtWidget::mouseMoveEvent(QMouseEvent* ev)
{
  if (!vl::OpenGLContext::mIgnoreNextMouseMoveEvent)
    dispatchMouseMoveEvent(ev->x(), ev->y());
  vl::OpenGLContext::mIgnoreNextMouseMoveEvent = false;
}

//-----------------------------------------------------------------------------

void VLQtWidget::mousePressEvent(QMouseEvent* ev)
{
  vl::EMouseButton bt = vl::NoButton;
  switch(ev->button())
  {
  case Qt::LeftButton:  bt = vl::LeftButton; break;
  case Qt::RightButton: bt = vl::RightButton; break;
  case Qt::MidButton:   bt = vl::MiddleButton; break;
  default:
    bt = vl::UnknownButton; break;
  }
  vl::OpenGLContext::dispatchMouseDownEvent(bt, ev->x(), ev->y());
}

//-----------------------------------------------------------------------------

void VLQtWidget::mouseReleaseEvent(QMouseEvent* ev)
{
  vl::EMouseButton bt = vl::NoButton;
  switch(ev->button())
  {
  case Qt::LeftButton:  bt = vl::LeftButton; break;
  case Qt::RightButton: bt = vl::RightButton; break;
  case Qt::MidButton:   bt = vl::MiddleButton; break;
  default:
    bt = vl::UnknownButton; break;
  }
  vl::OpenGLContext::dispatchMouseUpEvent(bt, ev->x(), ev->y());
}

//-----------------------------------------------------------------------------

void VLQtWidget::wheelEvent(QWheelEvent* ev)
{
  vl::OpenGLContext::dispatchMouseWheelEvent(ev->delta() / 120);
}

//-----------------------------------------------------------------------------

void VLQtWidget::keyPressEvent(QKeyEvent* ev)
{
  unsigned short unicode_ch = 0;
  vl::EKey key = vl::Key_None;
  translateKeyEvent(ev, unicode_ch, key);
  vl::OpenGLContext::dispatchKeyPressEvent(unicode_ch, key);
}

//-----------------------------------------------------------------------------

void VLQtWidget::keyReleaseEvent(QKeyEvent* ev)
{
  unsigned short unicode_ch = 0;
  vl::EKey key = vl::Key_None;
  translateKeyEvent(ev, unicode_ch, key);
  vl::OpenGLContext::dispatchKeyReleaseEvent(unicode_ch, key);
}

//-----------------------------------------------------------------------------

void VLQtWidget::translateKeyEvent(QKeyEvent* ev, unsigned short& unicode_out, vl::EKey& key_out)
{
  // translate non unicode characters
  key_out     = vl::Key_None;
  unicode_out = 0;

  switch(ev->key())
  {
    case Qt::Key_Clear:    key_out = vl::Key_Clear; break;
    case Qt::Key_Control:  key_out = vl::Key_Ctrl; break;
    // case Qt::Key_LCONTROL: key_out = Key_LeftCtrl; break;
    // case Qt::Key_RCONTROL: key_out = Key_RightCtrl; break;
    case Qt::Key_Alt:     key_out = vl::Key_Alt; break;
    // case Qt::Key_LMENU:    key_out = Key_LeftAlt; break;
    // case Qt::Key_RMENU:    key_out = Key_RightAlt; break;
    case Qt::Key_Shift:    key_out = vl::Key_Shift; break;
    // case Qt::Key_LSHIFT:   key_out = Key_LeftShift; break;
    // case Qt::Key_RSHIFT:   key_out = Key_RightShift; break;
    case Qt::Key_Insert:   key_out = vl::Key_Insert; break;
    case Qt::Key_Delete:   key_out = vl::Key_Delete; break;
    case Qt::Key_Home:     key_out = vl::Key_Home; break;
    case Qt::Key_End:      key_out = vl::Key_End; break;
    case Qt::Key_Print:    key_out = vl::Key_Print; break;
    case Qt::Key_Pause:    key_out = vl::Key_Pause; break;
    case Qt::Key_PageUp:   key_out = vl::Key_PageUp; break;
    case Qt::Key_PageDown: key_out = vl::Key_PageDown; break;
    case Qt::Key_Left:     key_out = vl::Key_Left; break;
    case Qt::Key_Right:    key_out = vl::Key_Right; break;
    case Qt::Key_Up:       key_out = vl::Key_Up; break;
    case Qt::Key_Down:     key_out = vl::Key_Down; break;
    case Qt::Key_F1:       key_out = vl::Key_F1; break;
    case Qt::Key_F2:       key_out = vl::Key_F2; break;
    case Qt::Key_F3:       key_out = vl::Key_F3; break;
    case Qt::Key_F4:       key_out = vl::Key_F4; break;
    case Qt::Key_F5:       key_out = vl::Key_F5; break;
    case Qt::Key_F6:       key_out = vl::Key_F6; break;
    case Qt::Key_F7:       key_out = vl::Key_F7; break;
    case Qt::Key_F8:       key_out = vl::Key_F8; break;
    case Qt::Key_F9:       key_out = vl::Key_F9; break;
    case Qt::Key_F10:      key_out = vl::Key_F10; break;
    case Qt::Key_F11:      key_out = vl::Key_F11; break;
    case Qt::Key_F12:      key_out = vl::Key_F12; break;

    case Qt::Key_0: key_out = vl::Key_0; break;
    case Qt::Key_1: key_out = vl::Key_1; break;
    case Qt::Key_2: key_out = vl::Key_2; break;
    case Qt::Key_3: key_out = vl::Key_3; break;
    case Qt::Key_4: key_out = vl::Key_4; break;
    case Qt::Key_5: key_out = vl::Key_5; break;
    case Qt::Key_6: key_out = vl::Key_6; break;
    case Qt::Key_7: key_out = vl::Key_7; break;
    case Qt::Key_8: key_out = vl::Key_8; break;
    case Qt::Key_9: key_out = vl::Key_9; break;

    case Qt::Key_A: key_out = vl::Key_A; break;
    case Qt::Key_B: key_out = vl::Key_B; break;
    case Qt::Key_C: key_out = vl::Key_C; break;
    case Qt::Key_D: key_out = vl::Key_D; break;
    case Qt::Key_E: key_out = vl::Key_E; break;
    case Qt::Key_F: key_out = vl::Key_F; break;
    case Qt::Key_G: key_out = vl::Key_G; break;
    case Qt::Key_H: key_out = vl::Key_H; break;
    case Qt::Key_I: key_out = vl::Key_I; break;
    case Qt::Key_J: key_out = vl::Key_J; break;
    case Qt::Key_K: key_out = vl::Key_K; break;
    case Qt::Key_L: key_out = vl::Key_L; break;
    case Qt::Key_M: key_out = vl::Key_M; break;
    case Qt::Key_N: key_out = vl::Key_N; break;
    case Qt::Key_O: key_out = vl::Key_O; break;
    case Qt::Key_P: key_out = vl::Key_P; break;
    case Qt::Key_Q: key_out = vl::Key_Q; break;
    case Qt::Key_R: key_out = vl::Key_R; break;
    case Qt::Key_S: key_out = vl::Key_S; break;
    case Qt::Key_T: key_out = vl::Key_T; break;
    case Qt::Key_U: key_out = vl::Key_U; break;
    case Qt::Key_V: key_out = vl::Key_V; break;
    case Qt::Key_W: key_out = vl::Key_W; break;
    case Qt::Key_X: key_out = vl::Key_X; break;
    case Qt::Key_Y: key_out = vl::Key_Y; break;
    case Qt::Key_Z: key_out = vl::Key_Z; break;
  }

  // fill unicode
  QString ustring = ev->text();
  if ( ustring.length() == 1 )
  {
    unicode_out = ustring[0].unicode();

    // fill key
    switch(unicode_out)
    {
      case L'0': key_out = vl::Key_0; break;
      case L'1': key_out = vl::Key_1; break;
      case L'2': key_out = vl::Key_2; break;
      case L'3': key_out = vl::Key_3; break;
      case L'4': key_out = vl::Key_4; break;
      case L'5': key_out = vl::Key_5; break;
      case L'6': key_out = vl::Key_6; break;
      case L'7': key_out = vl::Key_7; break;
      case L'8': key_out = vl::Key_8; break;
      case L'9': key_out = vl::Key_9; break;

      case L'A': key_out = vl::Key_A; break;
      case L'B': key_out = vl::Key_B; break;
      case L'C': key_out = vl::Key_C; break;
      case L'D': key_out = vl::Key_D; break;
      case L'E': key_out = vl::Key_E; break;
      case L'F': key_out = vl::Key_F; break;
      case L'G': key_out = vl::Key_G; break;
      case L'H': key_out = vl::Key_H; break;
      case L'I': key_out = vl::Key_I; break;
      case L'J': key_out = vl::Key_J; break;
      case L'K': key_out = vl::Key_K; break;
      case L'L': key_out = vl::Key_L; break;
      case L'M': key_out = vl::Key_M; break;
      case L'N': key_out = vl::Key_N; break;
      case L'O': key_out = vl::Key_O; break;
      case L'P': key_out = vl::Key_P; break;
      case L'Q': key_out = vl::Key_Q; break;
      case L'R': key_out = vl::Key_R; break;
      case L'S': key_out = vl::Key_S; break;
      case L'T': key_out = vl::Key_T; break;
      case L'U': key_out = vl::Key_U; break;
      case L'V': key_out = vl::Key_V; break;
      case L'W': key_out = vl::Key_W; break;
      case L'X': key_out = vl::Key_X; break;
      case L'Y': key_out = vl::Key_Y; break;
      case L'Z': key_out = vl::Key_Z; break;

      case L'a': key_out = vl::Key_A; break;
      case L'b': key_out = vl::Key_B; break;
      case L'c': key_out = vl::Key_C; break;
      case L'd': key_out = vl::Key_D; break;
      case L'e': key_out = vl::Key_E; break;
      case L'f': key_out = vl::Key_F; break;
      case L'g': key_out = vl::Key_G; break;
      case L'h': key_out = vl::Key_H; break;
      case L'i': key_out = vl::Key_I; break;
      case L'j': key_out = vl::Key_J; break;
      case L'k': key_out = vl::Key_K; break;
      case L'l': key_out = vl::Key_L; break;
      case L'm': key_out = vl::Key_M; break;
      case L'n': key_out = vl::Key_N; break;
      case L'o': key_out = vl::Key_O; break;
      case L'p': key_out = vl::Key_P; break;
      case L'q': key_out = vl::Key_Q; break;
      case L'r': key_out = vl::Key_R; break;
      case L's': key_out = vl::Key_S; break;
      case L't': key_out = vl::Key_T; break;
      case L'u': key_out = vl::Key_U; break;
      case L'v': key_out = vl::Key_V; break;
      case L'w': key_out = vl::Key_W; break;
      case L'x': key_out = vl::Key_X; break;
      case L'y': key_out = vl::Key_Y; break;
      case L'z': key_out = vl::Key_Z; break;

      case 13: key_out = vl::Key_Return; break;
      case 8: key_out = vl::Key_BackSpace; break;
      case 9: key_out = vl::Key_Tab; break;
      case L' ': key_out = vl::Key_Space; break;

      case 27: key_out = vl::Key_Escape; break;
      case L'!': key_out = vl::Key_Exclam; break;
      case L'"': key_out = vl::Key_QuoteDbl; break;
      case L'#': key_out = vl::Key_Hash; break;
      case L'$': key_out = vl::Key_Dollar; break;
      case L'&': key_out = vl::Key_Ampersand; break;
      case L'\'': key_out = vl::Key_Quote; break;
      case L'(': key_out = vl::Key_LeftParen; break;
      case L')': key_out = vl::Key_RightParen; break;
      case L'*': key_out = vl::Key_Asterisk; break;
      case L'+': key_out = vl::Key_Plus; break;
      case L',': key_out = vl::Key_Comma; break;
      case L'-': key_out = vl::Key_Minus; break;
      case L'.': key_out = vl::Key_Period; break;
      case L'\\': key_out = vl::Key_Slash; break;
      case L':': key_out = vl::Key_Colon; break;
      case L';': key_out = vl::Key_Semicolon; break;
      case L'<': key_out = vl::Key_Less; break;
      case L'=': key_out = vl::Key_Equal; break;
      case L'>': key_out = vl::Key_Greater; break;
      case L'?': key_out = vl::Key_Question; break;
      case L'@': key_out = vl::Key_At; break;
      case L'[': key_out = vl::Key_LeftBracket; break;
      case L'/': key_out = vl::Key_BackSlash; break;
      case L']': key_out = vl::Key_RightBracket; break;
      case L'|': key_out = vl::Key_Caret; break;
      case L'_': key_out = vl::Key_Underscore; break;
      case L'`': key_out = vl::Key_QuoteLeft; break;
    }
  }
}

//-----------------------------------------------------------------------------

#ifdef _USE_CUDA

void VLQtWidget::FreeCUDAInteropTextures()
{
  makeCurrent();

  for (std::map<mitk::DataNode::ConstPointer, TextureDataPOD>::iterator i = m_NodeToTextureMap.begin(); i != m_NodeToTextureMap.end(); )
  {
    if (i->second.m_CUDARes != 0)
    {
      cudaError_t err = cudaGraphicsUnregisterResource(i->second.m_CUDARes);
      if (err != cudaSuccess)
      {
        MITK_WARN << "Failed to unregister VL texture from CUDA";
      }
    }

    i = m_NodeToTextureMap.erase(i);
  }

  // if no cuda is available then this is most likely a nullptr.
  // and if not a nullptr then it's only a dummy. so unconditionally delete it.
  delete m_CUDAInteropPimpl;
  m_CUDAInteropPimpl = 0;

}

//-----------------------------------------------------------------------------

void VLQtWidget::EnableFBOCopyToDataStorageViaCUDA(bool enable, mitk::DataStorage* datastorage, const std::string& nodename)
{
  makeCurrent();

  if (enable)
  {
    if (datastorage == 0)
      throw std::runtime_error("Need data storage object");

    delete m_CUDAInteropPimpl;
    m_CUDAInteropPimpl = new CUDAInterop;
    m_CUDAInteropPimpl->m_FBOAdaptor = 0;
    m_CUDAInteropPimpl->m_DataStorage = datastorage;
    m_CUDAInteropPimpl->m_NodeName = nodename;
    if (m_CUDAInteropPimpl->m_NodeName.empty())
    {
      std::ostringstream    n;
      n << "0x" << std::hex << (void*) this;
      m_CUDAInteropPimpl->m_NodeName = n.str();
    }
  }
  else
  {
    delete m_CUDAInteropPimpl;
    m_CUDAInteropPimpl = 0;
  }
}

//-----------------------------------------------------------------------------

void VLQtWidget::PrepareBackgroundActor(const niftk::LightweightCUDAImage* lwci, const mitk::BaseGeometry* geom, const mitk::DataNode::ConstPointer node)
{
  makeCurrent();

  VIVID_CHECK(lwci != 0);

  vl::mat4  mat;
  mat = mat.setIdentity();
  ref<vl::Transform> tr     = new vl::Transform();
  tr->setLocalMatrix(mat);


  // essentially copied from vl::makeGrid()
  ref<vl::Geometry>         vlquad = new vl::Geometry;

  ref<vl::ArrayFloat3> vert3 = new vl::ArrayFloat3;
  vert3->resize(4);
  vlquad->setVertexArray(vert3.get());

  ref<vl::ArrayFloat2> text2 = new vl::ArrayFloat2;
  text2->resize(4);
  vlquad->setTexCoordArray(0, text2.get());

  //  0---3
  //  |   |
  //  1---2
  vert3->at(0).x() = -1; vert3->at(0).y() =  1; vert3->at(0).z() = 0;  text2->at(0).s() = 0; text2->at(0).t() = 0;
  vert3->at(1).x() = -1; vert3->at(1).y() = -1; vert3->at(1).z() = 0;  text2->at(1).s() = 0; text2->at(1).t() = 1;
  vert3->at(2).x() =  1; vert3->at(2).y() = -1; vert3->at(2).z() = 0;  text2->at(2).s() = 1; text2->at(2).t() = 1;
  vert3->at(3).x() =  1; vert3->at(3).y() =  1; vert3->at(3).z() = 0;  text2->at(3).s() = 1; text2->at(3).t() = 0;


  ref<vl::DrawElementsUInt> polys = new vl::DrawElementsUInt(vl::PT_QUADS);
  polys->indexBuffer()->resize(4);
  polys->indexBuffer()->at(0) = 0;
  polys->indexBuffer()->at(1) = 1;
  polys->indexBuffer()->at(2) = 2;
  polys->indexBuffer()->at(3) = 3;
  vlquad->drawCalls().push_back(polys.get());


  ref<vl::Effect>    fx = new vl::Effect;
  fx->shader()->disable(vl::EN_LIGHTING);
  // UpdateDataNode() takes care of assigning colour etc.

  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(vlquad.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );


  std::string   objName = actor->objectName() + "_background";
  actor->setObjectName(objName.c_str());

  m_NodeActorMap[node] = actor;
  m_NodeToTextureMap[node] = TextureDataPOD();
}

//-----------------------------------------------------------------------------

void VLQtWidget::UpdateGLTexturesFromCUDA(const mitk::DataNode::ConstPointer& node)
{
  makeCurrent();

  niftk::LightweightCUDAImage    lwcImage;

  niftk::CUDAImage::Pointer cudaimg = dynamic_cast<niftk::CUDAImage*>(node->GetData());
  if (cudaimg.IsNotNull())
  {
    lwcImage = cudaimg->GetLightweightCUDAImage();
  }
  else
  {
    mitk::Image::Pointer    img = dynamic_cast<mitk::Image*>(node->GetData());
    if (img.IsNotNull())
    {
      niftk::CUDAImageProperty::Pointer prop = dynamic_cast<niftk::CUDAImageProperty*>(img->GetProperty("CUDAImageProperty").GetPointer());
      if (prop.IsNotNull())
      {
        lwcImage = prop->Get();
      }
    }
  }

  NodeActorMapType::iterator     it = m_NodeActorMap.find(node);
  if (it == m_NodeActorMap.end())
    return;
  ref<vl::Actor>    actor = it->second;
  if (actor.get() == 0)
    return;

  if (lwcImage.GetId() != 0)
  {
    // whatever we had cached from a previous frame.
    TextureDataPOD          texpod    = m_NodeToTextureMap[node];

    // only need to update the vl texture, if content in our cuda buffer has changed.
    // and the cuda buffer can change only when we have a different id.
    if (texpod.m_LastUpdatedID != lwcImage.GetId())
    {
      cudaError_t   err = cudaSuccess;
      bool          neednewvltexture = texpod.m_Texture.get() == 0;

      // check if vl-texture size needs to change
      if (texpod.m_Texture.get() != 0)
      {
        neednewvltexture |= lwcImage.GetWidth()  != texpod.m_Texture->width();
        neednewvltexture |= lwcImage.GetHeight() != texpod.m_Texture->height();
      }

      if (neednewvltexture)
      {
        if (texpod.m_CUDARes)
        {
          err = cudaGraphicsUnregisterResource(texpod.m_CUDARes);
          texpod.m_CUDARes = 0;
          if (err != cudaSuccess)
          {
            MITK_WARN << "Could not unregister VL texture from CUDA. This will likely leak GPU memory.";
          }
        }

        texpod.m_Texture = new vl::Texture(lwcImage.GetWidth(), lwcImage.GetHeight(), vl::TF_RGBA8, false);
        actor->effect()->shader()->gocTextureSampler(0)->setTexture(texpod.m_Texture.get());
        actor->effect()->shader()->gocTextureSampler(0)->setTexParameter(m_DefaultTextureParams.get());

        err = cudaGraphicsGLRegisterImage(&texpod.m_CUDARes, texpod.m_Texture->handle(), GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
        if (err != cudaSuccess)
        {
          texpod.m_CUDARes = 0;
          MITK_WARN << "Registering VL texture into CUDA failed. Will not update (properly).";
        }
      }

      if (texpod.m_CUDARes)
      {
        VIVID_CHECK(actor->effect()->shader()->getTextureSampler(0)->texture() == texpod.m_Texture);

        niftk::CUDAManager*  cudamng   = niftk::CUDAManager::GetInstance();
        cudaStream_t         mystream  = cudamng->GetStream("VLQtWidget vl-texture update");
        niftk::ReadAccessor  inputRA   = cudamng->RequestReadAccess(lwcImage);

        // make sure producer of the cuda-image finished.
        err = cudaStreamWaitEvent(mystream, inputRA.m_ReadyEvent, 0);
        if (err != cudaSuccess)
        {
          // flood the log
          MITK_WARN << "cudaStreamWaitEvent failed with error code " << err;
        }

        // this also guarantees that ogl will have finished doing its thing before mystream starts copying.
        err = cudaGraphicsMapResources(1, &texpod.m_CUDARes, mystream);
        if (err == cudaSuccess)
        {
          // normally we would need to flip image! ogl is left-bottom, whereas everywhere else is left-top origin.
          // but texture coordinates that we have assigned to the quads rendering the current image will do that for us.

          cudaArray_t   arr = 0;
          err = cudaGraphicsSubResourceGetMappedArray(&arr, texpod.m_CUDARes, 0, 0);
          if (err == cudaSuccess)
          {
            err = cudaMemcpy2DToArrayAsync(arr, 0, 0, inputRA.m_DevicePointer, inputRA.m_BytePitch, lwcImage.GetWidth() * 4, lwcImage.GetHeight(), cudaMemcpyDeviceToDevice, mystream);
            if (err == cudaSuccess)
            {
              texpod.m_LastUpdatedID = lwcImage.GetId();
            }
          }

          err = cudaGraphicsUnmapResources(1, &texpod.m_CUDARes, mystream);
          if (err != cudaSuccess)
          {
            MITK_WARN << "Cannot unmap VL texture from CUDA. This will probably kill the renderer. Error code: " << err;
          }
        }
        // make sure Autorelease() and Finalise() are always the last things to do for a stream!
        // otherwise the streamcallback will block subsequent work.
        // in this case here, the callback managed by CUDAManager that keeps track of refcounts could stall
        // the opengl driver if cudaGraphicsUnmapResources() came after Autorelease().
        cudamng->Autorelease(inputRA, mystream);
      }

      // update cache, even if something went wrong.
      m_NodeToTextureMap[node] = texpod;

      // helps with debugging
      actor->effect()->shader()->disable(vl::EN_CULL_FACE);
    }
  }
}

//-----------------------------------------------------------------------------

ref<vl::Actor> VLQtWidget::AddCUDAImageActor(const mitk::BaseData* _cudaImg)
{
  makeCurrent();

  niftk::LightweightCUDAImage lwci;
  const niftk::CUDAImage* cudaImg = dynamic_cast<const niftk::CUDAImage*>(_cudaImg);
  if (cudaImg != 0)
  {
    lwci = cudaImg->GetLightweightCUDAImage();
  }
  else
  {
    niftk::CUDAImageProperty::Pointer prop = dynamic_cast<niftk::CUDAImageProperty*>(_cudaImg->GetProperty("CUDAImageProperty").GetPointer());
    if (prop.IsNotNull())
    {
      lwci = prop->Get();
    }
  }
  VIVID_CHECK(lwci.GetId() != 0);

  ref<vl::Transform> tr = new vl::Transform;
  UpdateTransformFromData(tr.get(), cudaImg);

  ref<vl::Geometry> vlquad    = CreateGeometryFor2DImage(lwci.GetWidth(), lwci.GetHeight());

  ref<vl::Effect>    fx = new vl::Effect;
  fx->shader()->disable(vl::EN_LIGHTING);
  fx->shader()->gocTextureSampler(1)->setTexture(m_DefaultTexture.get());
  fx->shader()->gocTextureSampler(1)->setTexParameter(m_DefaultTextureParams.get());
  // UpdateDataNode() takes care of assigning colour etc.

  ref<vl::Actor> actor = m_SceneManager->tree()->addActor(vlquad.get(), fx.get(), tr.get());
  actor->setEnableMask( vl::VividRenderer::DefaultEnableMask );

  return actor;

}
#endif