/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef itkBSplineBendingEnergyConstraint_h
#define itkBSplineBendingEnergyConstraint_h

#include "itkConstraint.h"
#include <itkUCLBSplineTransform.h>


namespace itk
{
  
/** 
 * \class BSplineBendingEnergyConstraint
 * \brief Calculated the bending energy, to be used as regulariser in FFD.
 *
 * In practice, you create this object, create your UCLBSplineTransform,
 * and inject the UCLBSplineTransform into this class.
 * When EvaluateContraint is called, this class delegates back to 
 * the UCLBSplineTransform, as the UCLBSplineTransform can calculate it's
 * own bending energy.
 * 
 * \ingroup RegistrationMetrics
 */
template <
    class TFixedImage,                   // Templated over the image type.
    class TScalarType,                   // Data type for scalars
    unsigned int NDimensions,            // Number of Dimensions i.e. 2D or 3D
    class TDeformationScalar             // The data type for the vector field
    >            
class ITK_EXPORT BSplineBendingEnergyConstraint : 
    public Constraint
{
public:
  /** Standard "Self" typedef. */
  typedef BSplineBendingEnergyConstraint Self;
  typedef Constraint                     Superclass;
  typedef SmartPointer<Self>             Pointer;
  typedef SmartPointer<const Self>       ConstPointer;
  typedef Superclass::MeasureType        MeasureType;
  typedef Superclass::DerivativeType     DerivativeType;
  typedef Superclass::ParametersType     ParametersType;

  /**  Type of the Transform . */
  typedef typename itk::UCLBSplineTransform<TFixedImage, TScalarType, NDimensions, TDeformationScalar > TransformType;
  typedef typename TransformType::Pointer                                                            TransformPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro( BSplineBendingEnergyConstraint, Constraint );

  /** Calculates the bending energy. */
  virtual MeasureType EvaluateConstraint(const ParametersType & parameters) override;

  /** Calculates the derivative of the bending energy. */
  virtual void EvaluateDerivative(const ParametersType & parameters, DerivativeType & derivative ) const override;
  
  /** Set/Get the Transfrom. */
  itkSetObjectMacro( Transform, TransformType );
  itkGetObjectMacro( Transform, TransformType );

protected:
  
  BSplineBendingEnergyConstraint();
  virtual ~BSplineBendingEnergyConstraint() {};

  void PrintSelf(std::ostream& os, Indent indent) const override;

private:  
  
  BSplineBendingEnergyConstraint(const Self&); // purposely not implemented
  void operator=(const Self&);                 // purposely not implemented

  /** The transform we are evaluating. */
  TransformPointer m_Transform;
  
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBSplineBendingEnergyConstraint.txx"
#endif

#endif
