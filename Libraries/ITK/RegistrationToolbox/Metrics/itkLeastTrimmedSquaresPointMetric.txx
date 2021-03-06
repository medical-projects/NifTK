/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef _itkLeastTrimmedSquaresPointMetric_txx
#define _itkLeastTrimmedSquaresPointMetric_txx

#include "itkLeastTrimmedSquaresPointMetric.h"

namespace itk
{

/** Constructor */
template <class TFixedPointSet, class TMovingPointSet> 
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>
::LeastTrimmedSquaresPointMetric() 
{
  m_PercentageOfPointsToKeep = 50;
}

/** Return the number of values, i.e the number of points in the moving set */
template <class TFixedPointSet, class TMovingPointSet>  
unsigned int
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>  
::GetNumberOfValues() const
{
 MovingPointSetConstPointer movingPointSet = this->GetMovingPointSet();

 if( !movingPointSet ) 
    {
      itkExceptionMacro( << "Moving point set has not been assigned" );
    }

 return  movingPointSet->GetPoints()->Size();
}


/** Get the match Measure */
template <class TFixedPointSet, class TMovingPointSet>  
typename LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>::MeasureType
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>
::GetValue( const TransformParametersType & parameters ) const
{
  FixedPointSetConstPointer fixedPointSet = this->GetFixedPointSet();

  if( !fixedPointSet ) 
    {
      itkExceptionMacro( << "Fixed point set has not been assigned" );
    }

  MovingPointSetConstPointer movingPointSet = this->GetMovingPointSet();

  if( !movingPointSet ) 
    {
      itkExceptionMacro( << "Moving point set has not been assigned" );
    }

  this->SetTransformParameters( parameters );

  PointIterator fixedPointItr = fixedPointSet->GetPoints()->Begin();
  PointIterator fixedPointEnd = fixedPointSet->GetPoints()->End();
  PointIterator movingPointItr = movingPointSet->GetPoints()->Begin();
  PointIterator movingPointEnd = movingPointSet->GetPoints()->End();
  
  double distance;
  unsigned long int i;

  std::priority_queue<double, std::vector<double>, std::greater<double> >  queue;
  
  while( fixedPointItr != fixedPointEnd && movingPointItr != movingPointEnd)
    {
      typename Superclass::InputPointType fixedPoint;
      fixedPoint.CastFrom( fixedPointItr.Value() );

      typename Superclass::InputPointType movingPoint;
      movingPoint.CastFrom( movingPointItr.Value() );
    
      typename Superclass::OutputPointType transformedPoint = 
        this->m_Transform->TransformPoint( fixedPoint );

      // Compute the squared distance.
      distance = 0;
      for (i = 0; i < TFixedPointSet::PointDimension; i++)
        {
          distance += ((movingPoint[i] - transformedPoint[i])*(movingPoint[i] - transformedPoint[i])); 
        }

      // Stick it in the queue.
      queue.push(distance);
      
      ++fixedPointItr;
      ++movingPointItr;
    }
  
  unsigned long int totalSize    = queue.size();
  unsigned long int numberToKeep = (unsigned long int)(totalSize * (m_PercentageOfPointsToKeep/100.0));
  
  double value        = 0;
  MeasureType measure = 0;
  
  i = 0;
  while (i < totalSize && i < numberToKeep)
    {
      value = queue.top();
      measure += value;
      queue.pop();
      i++;      
    }
  return measure;
}

/** Get the Derivative Measure */
template <class TFixedPointSet, class TMovingPointSet>
void
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>
::GetDerivative( const TransformParametersType & itkNotUsed(parameters),
                 DerivativeType & itkNotUsed(derivative) ) const
{

}

/** Get both the match Measure and theDerivative Measure  */
template <class TFixedPointSet, class TMovingPointSet>  
void
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>
::GetValueAndDerivative(const TransformParametersType & parameters, 
                        MeasureType & value, DerivativeType  & derivative) const
{
  value = this->GetValue(parameters);
  this->GetDerivative(parameters,derivative);
}

/** PrintSelf method */
template <class TFixedPointSet, class TMovingPointSet>  
void
LeastTrimmedSquaresPointMetric<TFixedPointSet,TMovingPointSet>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}

} // end namespace itk


#endif
