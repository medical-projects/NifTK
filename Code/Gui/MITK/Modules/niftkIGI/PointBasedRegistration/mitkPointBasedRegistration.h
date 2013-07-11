/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef mitkPointBasedRegistration_h
#define mitkPointBasedRegistration_h

#include "niftkIGIExports.h"
#include <mitkDataStorage.h>
#include <vtkMatrix4x4.h>
#include <mitkPointSet.h>
#include <mitkDataNode.h>
#include <itkObject.h>
#include <itkObjectFactoryBase.h>

namespace mitk {

/**
 * \class PointBasedRegistration
 * \brief Class to implement point based registration of two point sets.
 */
class NIFTKIGI_EXPORT PointBasedRegistration : public itk::Object
{
public:

  mitkClassMacro(PointBasedRegistration, itk::Object);
  itkNewMacro(PointBasedRegistration);

  /**
   * \brief Stores the default value of whether to use ICP initialisation = false.
   */
  static const bool DEFAULT_USE_ICP_INITIALISATION;

  /**
   * \brief Main method to calculate the point based registration.
   * \param[In] fixedPointSet a point set
   * \param[In] movingPointSet a point set
   * \param[In,Out] useICPInitialisation if true, will compute closest point pairs, so the
   * number of points in each data set can be different, but does require at least 6 points in
   * each data set, and if false will assume that the point sets are ordered, of equal size,
   * and with points corresponding.
   * \param[In,Out] the transformation to transform the moving point set into the coordinate system of the fixed point set.
   * \return Returns the Fiducial Registration Error
   */
  double Update(const mitk::PointSet::Pointer fixedPointSet,
              const mitk::PointSet::Pointer movingPointSet,
              const bool& useICPInitialisation,
              vtkMatrix4x4& outputTransform) const;

protected:

  PointBasedRegistration(); // Purposefully hidden.
  virtual ~PointBasedRegistration(); // Purposefully hidden.

  PointBasedRegistration(const PointBasedRegistration&); // Purposefully not implemented.
  PointBasedRegistration& operator=(const PointBasedRegistration&); // Purposefully not implemented.

private:

}; // end class

} // end namespace

#endif
