/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef _itkPerspectiveProjectionTransform_txx
#define _itkPerspectiveProjectionTransform_txx

#include "itkPerspectiveProjectionTransform.h"

#include <itkUCLMacro.h>

namespace itk
{

// Constructor with default arguments
template<class TScalarType>
PerspectiveProjectionTransform<TScalarType>::
PerspectiveProjectionTransform() : Superclass(ParametersDimension)
{
  this->m_Parameters[0] = 1000.;
  this->m_Parameters[1] = 0.;
  this->m_Parameters[2] = 0.;

  m_k1 = 1.;
  m_k2 = 1.;
}
 

// Destructor
template<class TScalarType>
PerspectiveProjectionTransform<TScalarType>::
~PerspectiveProjectionTransform()
{

}


// Print self
template<class TScalarType>
void
PerspectiveProjectionTransform<TScalarType>::
PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Focal distance: "<< this->m_Parameters[0] << std::endl;
  os << indent << "2D projection origin: " << this->m_Parameters[1] << ", " << this->m_Parameters[2] << std::endl;
  os << indent << "k1, k2: " << this->m_k1 << ", " << this->m_k2 << std::endl;
}


// Set Parameters
template <class TScalarType>
void
PerspectiveProjectionTransform<TScalarType>
::SetParameters( const ParametersType & parameters )
{
  niftkitkDebugMacro(<< "Setting parameters " << parameters );

  this->m_Parameters[0] = parameters[0];
  this->m_Parameters[1] = parameters[1];
  this->m_Parameters[2] = parameters[2];

  // Modified is always called since we just have a pointer to the
  // parameters and cannot know if the parameters have changed.
  this->Modified();

}


// Get Parameters
template <class TScalarType>
const typename PerspectiveProjectionTransform<TScalarType>::ParametersType &
PerspectiveProjectionTransform<TScalarType>
::GetParameters() const
{
  return this->m_Parameters;
}


// Return the perspective transformation matrix
template<class TScalarType>
typename PerspectiveProjectionTransform<TScalarType>::MatrixType
PerspectiveProjectionTransform<TScalarType>::
GetMatrix() const 
{
  MatrixType perspMatrix;

  perspMatrix(0, 0) =  this->m_Parameters[0]*m_k1;
  perspMatrix(0, 1) = 0.;
  perspMatrix(0, 2) = this->m_Parameters[1];
  perspMatrix(0, 3) = 0.;

  perspMatrix(1, 0) = 0.;
  perspMatrix(1, 1) = this->m_Parameters[0]*m_k2;
  perspMatrix(1, 2) = this->m_Parameters[2];
  perspMatrix(1, 3) = 0.;

  perspMatrix(2, 0) = 0.;
  perspMatrix(2, 1) = 0.;
  perspMatrix(2, 2) = 1.;
  perspMatrix(2, 3) = 0.;

  perspMatrix(3, 0) = 0.;
  perspMatrix(3, 1) = 0.;
  perspMatrix(3, 2) = 1.;
  perspMatrix(3, 3) = 0.;

  return perspMatrix;
}


// Transform a point
template<class TScalarType>
typename PerspectiveProjectionTransform<TScalarType>::OutputPointType
PerspectiveProjectionTransform<TScalarType>::
TransformPoint(const InputPointType &point) const 
{
  TScalarType factor = 1.;
  OutputPointType result;

  std::cout << "PerspectiveProjectionTransform::TransformPoint: " << point << std::endl;

  if ( point[2] )
    factor = this->m_Parameters[0] / point[2];
  
  result[0] = point[0]*factor*m_k1 + this->m_Parameters[1];
  result[1] = point[1]*factor*m_k2 + this->m_Parameters[2];

  return result;
}

 
// Compute the Jacobian in one position 
template<class TScalarType >
const typename PerspectiveProjectionTransform<TScalarType>::JacobianType 
PerspectiveProjectionTransform< TScalarType >::
GetJacobian( const InputPointType &p) const
{
  JacobianType tmp;
  this->ComputeJacobianWithRespectToParameters( p, tmp );
  return tmp;	
}
 
template<class TScalarType>
void
PerspectiveProjectionTransform<TScalarType>
::ComputeJacobianWithRespectToParameters( const InputPointType &p, JacobianType &jacobian ) const
{
  jacobian.Fill( 0.0 );

  // TODO
}



} // namespace

#endif
