/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkVLUtils_h
#define niftkVLUtils_h

#include <niftkVLExports.h>
#include <vlCore/Vector3.hpp>
#include <vlCore/Vector4.hpp>
#include <vlCore/vlnamespace.hpp>
#include <vlCore/Image.hpp>
#include <vlGraphics/Geometry.hpp>
#include <vlVivid/VividVolume.hpp>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <itkMatrix.h>

// NOTE:
// In production you may want to review these checks and handle them as gracefully as possible.
#if defined(_MSC_VER)
  #define VIVID_TRAP() { if (IsDebuggerPresent()) { __debugbreak(); } else ::vl::abort_vl(); }
#elif defined(__GNUG__) || defined(__MINGW32__)
  #define VIVID_TRAP() { fflush(stdout); fflush(stderr); asm("int $0x3"); }
#else
  #define VIVID_TRAP() { ::vl::abort_vl(); }
#endif
#if 1
  // This is better for debugging
  #define VIVID_CHECK(expr) { if ( ! ( expr ) ) { ::vl::log_failed_check( #expr, __FILE__, __LINE__ ); VIVID_TRAP(); } }
#else
  // This allows the user to ignore the exception while giving full info about the error
  #define STRINGIZE_DETAIL(x) #x
  #define STRINGIZE(x) STRINGIZE_DETAIL(x)
  #define VIVID_CHECK(expr) { if ( ! (expr) ) { throw std::runtime_error( __FILE__ " line " STRINGIZE(__LINE__) ": " #expr ); } }
#endif

class vtkPolyData;

namespace mitk
{
  class BaseData;
  class DataNode;
  class Image;
}

namespace vl
{
  class VividVolume;
  class Effect;
}

namespace niftk
{

class NIFTKVL_EXPORT VLUserData: public vl::Object
{
public:
  VLUserData(): m_TransformModifiedTime(0), m_ImageModifiedTime(0) { }
  itk::ModifiedTimeType m_TransformModifiedTime;
  itk::ModifiedTimeType m_ImageModifiedTime;
};

class NIFTKVL_EXPORT VLUtils
{
public:
  static vl::vec3 getVector3DProp( const mitk::DataNode* node, const char* prop_name, vl::vec3 defval );

  static vl::vec3 getPoint3DProp( const mitk::DataNode* node, const char* prop_name, vl::vec3 defval );

  static vl::vec4 getPoint4DProp( const mitk::DataNode* node, const char* prop_name, vl::vec4 defval );

  static int getEnumProp( const mitk::DataNode* node, const char* prop_name, int defval = 0 );

  static bool getBoolProp( const mitk::DataNode* node, const char* prop_name, bool defval );

  static bool setBoolProp( mitk::DataNode* node, const char* prop_name, bool val );

  static float getFloatProp( const mitk::DataNode* node, const char* prop_name, float defval = 0 );

  static int getIntProp( const mitk::DataNode* node, const char* prop_name, int defval = 0 );

  static vl::vec4 getColorProp( const mitk::DataNode* node, const char* prop_name, vl::vec4 defval = vl::white );

  static void initGlobalProps( mitk::DataNode* node );

  static void initVolumeProps( mitk::DataNode* node );

  static void updateVolumeProps( vl::VividVolume* vol, const mitk::DataNode* node );

  static void initMaterialProps( mitk::DataNode* node );

  static void updateMaterialProps( vl::Effect* fx, const mitk::DataNode* node );

  static void initFogProps( mitk::DataNode* node );

  static void updateFogProps( vl::Effect* fx, const mitk::DataNode* node );

  static void initClipProps( mitk::DataNode* node );

  static void updateClipProps( vl::Effect* fx, const mitk::DataNode* node );

  static void initRenderModeProps( mitk::DataNode* node );

  static void updateRenderModeProps( vl::Effect* fx, const mitk::DataNode* node );

  static vl::EImageType mapITKPixelTypeToVL(int itkComponentType);

  static vl::EImageFormat mapComponentsToVLColourFormat(int components);

  static vl::ref<vl::Image> wrapMitk2DImage( const mitk::Image* mitk_image );

  static VLUserData* getUserData(vl::Actor* actor);

  static vl::mat4 getVLMatrix(const itk::Matrix<float, 4, 4>& itkmat);

  static vl::mat4 getVLMatrix(vtkSmartPointer<vtkMatrix4x4> vtkmat);

  static vl::mat4 getVLMatrix(const mitk::BaseData* data);

  static void updateTransform(vl::Transform* tr, const mitk::BaseData* data);

  static void updateTransform(vl::Transform* tr, const mitk::DataNode* node);

  static vl::ref<vl::Geometry> make2DImageGeometry(int width, int height);

  static vl::ref<vl::Geometry> getVLGeometry(vtkPolyData* vtkPoly);

  static void dumpNodeInfo( const std::string& prefix, const mitk::DataNode* node );

  static void dumpNodeInfo( const std::string& prefix, const mitk::BaseData* data );
};

}

#endif

