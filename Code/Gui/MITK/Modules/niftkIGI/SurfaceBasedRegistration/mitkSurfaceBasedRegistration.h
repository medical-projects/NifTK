/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef mitkSurfaceBasedRegistration_h
#define mitkSurfaceBasedRegistration_h

#include "niftkIGIExports.h"
#include <mitkDataStorage.h>
#include <vtkMatrix4x4.h>
#include <mitkDataNode.h>
#include <mitkSurface.h>
#include <mitkPointSet.h>
#include <itkObject.h>
#include <itkObjectFactoryBase.h>
#include <vtkPolyData.h>

namespace mitk {

/**
 * \class SurfaceBasedRegistration
 * \brief Class to perform a surface based registration of two MITK Surfaces/PointSets.
 */
class NIFTKIGI_EXPORT SurfaceBasedRegistration : public itk::Object
{
public:

  mitkClassMacro(SurfaceBasedRegistration, itk::Object);
  itkNewMacro(SurfaceBasedRegistration);

  enum Method 
  {
    VTK_ICP, //VTK's ICP algorithm, point to surface
    DEFORM //A hypothetical non rigid point to surface algorithm
  };

  static const int DEFAULT_MAX_ITERATIONS;
  static const int DEFAULT_MAX_POINTS;
  static const bool DEFAULT_USE_DEFORMABLE;
  static const bool DEFAULT_USE_SPATIALFILTER;
  /**
   * \brief Write My Documentation
   */
  void Update(const mitk::DataNode* fixedNode,
           const mitk::DataNode* movingNode,
           vtkMatrix4x4* transformMovingToFixed);

  /**
   * \brief apply the transform to a given data node
   */
  void ApplyTransform (mitk::DataNode::Pointer node);
  void ApplyTransform (mitk::DataNode::Pointer node, vtkMatrix4x4* transform);
  static void GetCurrentTransform ( const mitk::DataNode * node , vtkMatrix4x4* matrix );

  itkSetMacro (MaximumIterations, int);
  itkSetMacro (MaximumNumberOfLandmarkPointsToUse, int);
  itkSetMacro (Method, Method);
  itkSetMacro (UseSpatialFilter, bool);


  /**
   * \brief An crude method to spatially filter the mitk point cloud, assuming that the 
   * cloud is from a surface reconstruction where the point id encodes the point's 
   * position on screen
   */
  static void PointSetToPolyData_SpatialFilter ( const mitk::PointSet::Pointer PointsIn, vtkPolyData* PolyOut);
  static void NodeToPolyData ( const mitk::DataNode* , vtkPolyData* PolyOut, bool useSpatialFilter = false);
  static void PointSetToPolyData ( const mitk::PointSet::Pointer PointsIn, vtkPolyData* PolyOut);

protected:

  SurfaceBasedRegistration(); // Purposefully hidden.
  virtual ~SurfaceBasedRegistration(); // Purposefully hidden.

  SurfaceBasedRegistration(const SurfaceBasedRegistration&); // Purposefully not implemented.
  SurfaceBasedRegistration& operator=(const SurfaceBasedRegistration&); // Purposefully not implemented.

private:

  int m_MaximumIterations;
  int m_MaximumNumberOfLandmarkPointsToUse;
  Method m_Method;
  bool m_UseSpatialFilter;  //flag to control use of spatial filter

  vtkMatrix4x4* m_Matrix;


  void RunVTKICP(vtkPolyData* fixedPoly,
           vtkPolyData* movingPoly,
           vtkMatrix4x4* transformMovingToFixed);
}; // end class

} // end namespace

#endif
