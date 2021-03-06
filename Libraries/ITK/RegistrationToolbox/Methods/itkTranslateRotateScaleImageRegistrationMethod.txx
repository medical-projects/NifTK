/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef _itkTranslateRotateScaleImageRegistrationMethod_txx
#define _itkTranslateRotateScaleImageRegistrationMethod_txx

#include "itkTranslateRotateScaleImageRegistrationMethod.h"

namespace itk
{
/*
 * Constructor
 */
template < typename TInputImageType >
TranslateRotateScaleImageRegistrationMethod<TInputImageType>
::TranslateRotateScaleImageRegistrationMethod()
{
  niftkitkDebugMacro(<<"Constructed:TranslateRotateScaleImageRegistrationMethod");
}

/*
 * The optimize bit that we can now override.
 */
template < typename TInputImageType >
void
TranslateRotateScaleImageRegistrationMethod<TInputImageType>
::DoMultiStageRegistration(OptimizerPointer optimizer, TransformPointer transform)  throw (ExceptionObject)
{
  niftkitkDebugMacro(<<"TranslateRotateScaleImageRegistrationMethod::DoRegistration::Start");
  
  int i = 0;
  int currentNumberOfDof;
  
  double maxStepSize = optimizer->GetMaximumStepLength();
  double minStepSize = optimizer->GetMinimumStepLength();
  double currentStepSize = maxStepSize;
  double reductionFactor = this->GetLoopStepSizeReductionFactor();
  double tolerance = this->GetLoopTolerance();
  int    numberOfIterations = this->GetMaxNumberOfLoops();
  
  double translationMetricValue = 0;
  double rotationMetricValue = 0;
  double scaleMetricValue = 0;
  
  ParametersType params;
  
  ScalesType scales;
  scales.Fill(1);

  i = 0;  
  do
    {

      do
        {
          
          transform->SetFullAffine();
          params = transform->GetParameters();          

          niftkitkDebugMacro(<<"Start of loop:" << i << ", params:" << params << ", currentStepSize:" <<  currentStepSize);
      
          // We do translation and rotation without brain mask.
          this->SetUseFixedMask(false);
          this->SetUseMovingMask(false);
          this->Initialize();

          transform->SetJustTranslation();
          currentNumberOfDof = transform->GetNumberOfDOF();
          
          niftkitkDebugMacro(<<"Before translation, parameters are:" << transform->GetParameters() << ", dof:" << currentNumberOfDof << std::endl);
      
          scales.SetSize(currentNumberOfDof);
          scales.Fill(1);
          
          optimizer->SetInitialPosition(transform->GetParameters());      
          optimizer->SetScales(scales);
          optimizer->SetMaximumStepLength(currentStepSize);
          optimizer->SetMinimumStepLength(currentStepSize);
          optimizer->StartOptimization();

          transform->SetParameters(optimizer->GetCurrentPosition());
          translationMetricValue = optimizer->GetValue();
          
          transform->SetFullAffine();
          params = transform->GetParameters();            
          niftkitkDebugMacro(<<"After translation:" << i << ", params:" << params << ", value:" << translationMetricValue);
      
          // Do rotations
          transform->SetJustRotation();
          currentNumberOfDof = transform->GetNumberOfDOF();

          niftkitkDebugMacro(<<"Before rotation, parameters are:" << transform->GetParameters()  << ", dof:" << currentNumberOfDof << std::endl);
      
          scales.SetSize(currentNumberOfDof);
          scales.Fill(1);
          
          optimizer->SetInitialPosition(transform->GetParameters());
          optimizer->SetScales(scales);
          optimizer->SetMaximumStepLength(currentStepSize);
          optimizer->SetMinimumStepLength(currentStepSize);          
          optimizer->StartOptimization();
          
          transform->SetParameters(optimizer->GetCurrentPosition());
          rotationMetricValue = optimizer->GetValue();
          
          transform->SetFullAffine();
          params = transform->GetParameters();   
          niftkitkDebugMacro(<<"After rotation:" << i << ", params:" << params << ", value:" << rotationMetricValue);

          // Do scale with dilated brain mask.
          this->SetUseFixedMask(true);
          this->SetUseMovingMask(true);
          this->Initialize();
          
          transform->SetJustScale();
          currentNumberOfDof = transform->GetNumberOfDOF();
          
          niftkitkDebugMacro(<<"Before scale, parameters are:" << transform->GetParameters()  << ", dof:" << currentNumberOfDof << std::endl);
      
          scales.SetSize(currentNumberOfDof);
          scales.Fill(100);
          
          optimizer->SetInitialPosition(transform->GetParameters());
          optimizer->SetScales(scales);
          optimizer->SetMaximumStepLength(currentStepSize);
          optimizer->SetMinimumStepLength(currentStepSize);          
          optimizer->StartOptimization();
          transform->SetParameters(optimizer->GetCurrentPosition());
          scaleMetricValue = optimizer->GetValue();
          
          transform->SetFullAffine();
          params = transform->GetParameters();   
          niftkitkDebugMacro(<<"After scale:" << i << ", params:" << params << ", value:" << scaleMetricValue);
      
          i++;  
        }
      while (
               (fabs(translationMetricValue - rotationMetricValue) > tolerance 
              ||fabs(rotationMetricValue - scaleMetricValue) > tolerance
              ||fabs(scaleMetricValue - translationMetricValue) > tolerance)
               && i < numberOfIterations);
      
      currentStepSize *= reductionFactor;
    }
  while (currentStepSize > minStepSize);
  
  niftkitkDebugMacro(<<"TranslateRotateScaleImageRegistrationMethod::DoRegistration::Finish");
}

} // end namespace itk


#endif
