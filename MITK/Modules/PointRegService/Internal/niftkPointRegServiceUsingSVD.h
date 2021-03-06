/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkPointRegServiceUsingSVD_h
#define niftkPointRegServiceUsingSVD_h

#include <niftkPointRegServiceI.h>

namespace niftk
{

/**
* @class PointRegServiceUsingSVD
* @brief Implements niftk::PointRegServiceI using niftk::PointBasedRegistrationUsingSVD.
*/
class PointRegServiceUsingSVD : public niftk::PointRegServiceI
{
public:

  PointRegServiceUsingSVD();
  ~PointRegServiceUsingSVD();

  /**
  * @see niftk::PointBasedRegistrationUsingSVD
  * @throws mitk::Exception for all errors
  */
  virtual double Register(const mitk::PointSet::Pointer fixedPoints,
                          const mitk::PointSet::Pointer movingPoints,
                          vtkMatrix4x4& matrix) const override;

private:

  PointRegServiceUsingSVD(const PointRegServiceUsingSVD&); // deliberately not implemented
  PointRegServiceUsingSVD& operator=(const PointRegServiceUsingSVD&); // deliberately not implemented

};

} // end namespace

#endif
