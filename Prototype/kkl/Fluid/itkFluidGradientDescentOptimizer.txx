/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.
 
 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-12-15 14:34:51 +0000 (Thu, 15 Dec 2011) $
 Revision          : $Revision: 8026 $
 Last modified by  : $Author: kkl $
 
 Original author   : leung@drc.ion.ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details. 

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/
#ifndef ITKFluidGradientDescentOptimizer_TXX_
#define ITKFluidGradientDescentOptimizer_TXX_

#include "itkFluidGradientDescentOptimizer.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkImageRandomNonRepeatingIteratorWithIndex.h"
#include "itkLogHelper.h"

namespace itk
{
template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::FluidGradientDescentOptimizer()
{
  this->m_StartingStepSize = 0.0;
  this->m_NormaliseStepSize = true; 
  this->m_MinimumDeformationMaximumIterations = -1; 
  this->m_CurrentMinimumDeformationIterations = 0; 
  this->m_CalculateVelocityFeild = true; 
  this->m_IsSymmetric = false; 
  
  this->m_RegriddingFixedImageResampler = Superclass::ResampleFilterType::New();
  this->m_AdjustedTimeStep = 0.; 
  this->m_AdjustedFixedImageTimeStep = 0.; 
  this->m_AsgdWFudgeFactor = 1.; 
  this->m_AsgdFMinFudgeFactor = 1.; 
  this->m_AsgdA = 2.; 
  
  niftkitkDebugMacro(<< "FluidGradientDescentOptimizer():Constructed");
}

template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
void
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::StartOptimization( void )
{
  niftkitkDebugMacro(<< "StartFluidOptimization():");
  
  this->m_StartingStepSize = this->m_StepSize; 
  this->m_NormaliseStepSize = true; 
  this->m_CurrentMinimumDeformationIterations = 0; 
  this->m_CalculateVelocityFeild = true; 
  this->m_PreviousGradient = NULL; 
  this->m_AdjustedTimeStep = 0.; 
  this->m_AdjustedFixedImageTimeStep = 0.; 
  this->m_PreviousFixedImageGradient = NULL; 
  this->m_StepSizeHistoryForMovingImage.clear(); 
  this->m_StepSizeHistoryForMovingImage.reserve(this->m_MaximumNumberOfIterations+1); 
  this->m_StepSizeHistoryForFixedImage.clear(); 
  this->m_StepSizeHistoryForFixedImage.reserve(this->m_MaximumNumberOfIterations+1); 
  
  if (this->m_DeformableTransform.IsNull())
    {
      itkExceptionMacro(<< "The deformable transform is null, please inject one.");
    }
  
  this->m_ImageToImageMetric = dynamic_cast<typename Superclass::ImageToImageMetricPointer>(this->GetCostFunction());
  if (this->m_ImageToImageMetric == 0)
    {
      itkExceptionMacro(<< "Cannot cast image to image metric.");
    }
  
  this->m_FixedImage = const_cast<typename Superclass::FixedImagePointer>(this->m_ImageToImageMetric->GetFixedImage());
  if (this->m_FixedImage == 0)
    {
      itkExceptionMacro(<< "Cannot cast fixed image");
    }
  
  this->m_MovingImage = const_cast<typename Superclass::MovingImagePointer>(this->m_ImageToImageMetric->GetMovingImage());
  if (this->m_MovingImage == 0)
    {
      itkExceptionMacro(<< "Cannot cast moving image");
    }

  this->m_CurrentIteration = 0;
  
  typename Superclass::ScalesType scales(1);
  scales.Fill(1.0);
  this->SetScales(scales);
  
  if (GetInitialDeformableParameters().IsNull())
  {
    this->m_CurrentDeformableParameters = GetFluidDeformableTransform()->GetDeformableParameters(); 
  }
  else
  {
    this->m_CurrentDeformableParameters = GetInitialDeformableParameters(); 
  }
  this->m_NextDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters); 
  this->m_RegriddedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters); 
  
  if (this->m_IsSymmetric)
  {
    this->m_CurrentFixedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters); 
    this->m_NextFixedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters);
    this->m_RegriddedFixedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters);
  }
  
  this->m_MovingImageTransformComposedJacobianForward = NULL; 
  this->m_MovingImageTransformComposedJacobianBackward = NULL; 
  this->m_FixedImageTransformComposedJacobianForward = NULL; 
  this->m_FixedImageTransformComposedJacobianBackward = NULL; 
  
  this->ResumeOptimization();
}

/*
 * PrintSelf
 */
template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
void
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf( os, indent );
  
  if (!m_ForceFilter.IsNull())
    {
      os << indent << "ForceFilter=" << m_ForceFilter << std::endl;
    }
  if (!m_FluidPDESolver.IsNull())
    {
      os << indent << "FluidPDESolver=" << m_FluidPDESolver << std::endl;
    }
  if (!m_FluidVelocityToDeformationFilter.IsNull())
    {
      os << indent << "FluidVelocityToDeformationFilter=" << m_FluidVelocityToDeformationFilter << std::endl;
    }
}

template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
double
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::CalculateNextStep(int iterationNumber, double currentSimilarity, typename DeformableTransformType::DeformableParameterPointerType current, typename DeformableTransformType::DeformableParameterPointerType & next, typename DeformableTransformType::DeformableParameterPointerType currentFixed, typename DeformableTransformType::DeformableParameterPointerType & nextFixed)
{
  niftkitkInfoMacro(<< "CalculateNextStep():Started");
  double bestFixedImageStepSize = 0.; 
  
  if (m_ForceFilter.IsNull())
    {
      itkExceptionMacro(<< "Force filter is not set");
    } 
  if (m_FluidPDESolver.IsNull())
    {
      itkExceptionMacro(<< "PDE filter is not set");
    } 
  if (m_FluidVelocityToDeformationFilter.IsNull())
    {
      itkExceptionMacro(<< "FluidVelocityToDeformationFilter filter is not set");
    } 

  FluidTransformPointer transform = GetFluidDeformableTransform(); 

  // Set the current parameter/deformation. 
  transform->SetDeformableParameters(current);
  
  if (this->m_CalculateVelocityFeild)
  {
    // Force is calculated between fixed, and regridded moving image.
    if (!this->m_IsSymmetric)
    {
      m_ForceFilter->SetFixedImage(this->m_FixedImage);
    }
    else
    {
      m_ForceFilter->SetFixedImage(this->m_ImageToImageMetric->GetTransformedFixedImage());
    }
    m_ForceFilter->SetIsSymmetric(this->m_IsSymmetric); 
    m_ForceFilter->SetTransformedMovingImage(this->m_ImageToImageMetric->GetTransformedMovingImage());
    //m_ForceFilter->SetUnTransformedMovingImage(this->m_MovingImage);
    m_ForceFilter->SetUnTransformedMovingImage(this->m_ImageToImageMetric->GetTransformedMovingImage());
    m_ForceFilter->SetFixedImageMask(this->m_ImageToImageMetric->GetFixedImageMask()); 
    m_ForceFilter->Modified();
    niftkitkInfoMacro(<< "CalculateNextStep():Updating force");
    m_ForceFilter->UpdateLargestPossibleRegion();
    
    // Output force image. 
    //typedef ImageFileWriter<typename ForceFilterType::OutputImageType> ForceWriterType; 
    //typename ForceWriterType::Pointer forceWriter = ForceWriterType::New(); 
    //forceWriter->SetInput(m_ForceFilter->GetOutput()); 
    //forceWriter->SetFileName("force.nii"); 
    //forceWriter->Update(); 
    
    m_FluidPDESolver->SetIsSymmetric(this->m_IsSymmetric); 
    m_FluidPDESolver->SetInput(this->m_ForceFilter->GetOutput());
    niftkitkInfoMacro(<< "CalculateNextStep():Updating solver");
    m_FluidPDESolver->UpdateLargestPossibleRegion();
    this->m_CurrentVelocityField = m_FluidPDESolver->GetOutput(); 
    this->m_CurrentVelocityField->DisconnectPipeline(); 
    
    // Output velocity image. 
    //typedef ImageFileWriter<typename FluidPDEType::OutputImageType> FluidWriterType; 
    //typename FluidWriterType::Pointer fluidWriter = FluidWriterType::New(); 
    //fluidWriter->SetInput(this->m_CurrentVelocityField); 
    //fluidWriter->SetFileName("velocity.nii"); 
    //fluidWriter->Update(); 
  }
  else
  {
    niftkitkInfoMacro(<< "CalculateNextStep(): after regridding - keep the same velocity field.");
    this->m_CalculateVelocityFeild = true; 
  }
    
  niftkitkInfoMacro(<< "CalculateNextStep():solver done");
  // Delta must be set before injecting m_FluidVelocityToDeformationFilter into this class.
  // m_FluidVelocityToDeformationFilter->SetInputMask(this->m_AsgdMask); 
  m_FluidVelocityToDeformationFilter->SetCurrentDeformationField(transform->GetDeformationField());
  m_FluidVelocityToDeformationFilter->SetVelocityField(this->m_CurrentVelocityField);
  if (this->m_IsSymmetric)
  {
    m_FluidVelocityToFixedImageDeformationFilter->SetIsNegativeVelocity(true); 
    // m_FluidVelocityToFixedImageDeformationFilter->SetInputMask(this->m_AsgdMask); 
    m_FluidVelocityToFixedImageDeformationFilter->SetCurrentDeformationField(m_FixedImageTransform->GetDeformationField()); 
    m_FluidVelocityToFixedImageDeformationFilter->SetVelocityField(this->m_CurrentVelocityField); 
    m_FluidVelocityToFixedImageDeformationFilter->UpdateLargestPossibleRegion();
  }
  // Make it happen.
  niftkitkInfoMacro(<< "CalculateNextStep():Updating deformation");
  m_FluidVelocityToDeformationFilter->UpdateLargestPossibleRegion();
  niftkitkInfoMacro(<< "CalculateNextStep():max perturbation=" << this->m_FluidVelocityToDeformationFilter->GetMaxDeformation());
  
  // Estimate the variance of the error/noise in the gradient/velocity measurement by random sampling of the images. 
  double eta = 0.; 
  if (this->m_AsgdW < 0)
  {
    this->m_AsgdW = EstimateGradientErrorVariance(2000, 10, this->m_CurrentVelocityField, &eta)*this->m_AsgdWFudgeFactor; 
    niftkitkInfoMacro(<<"Estimated this->m_AsgdW=" << this->m_AsgdW); 
  }
  if (this->m_AsgdFMin > 0)
  {
    this->m_AsgdFMin = eta-1; 
    niftkitkInfoMacro(<<"Estimated this->m_AsgdFMin=" << this->m_AsgdFMin); 
  }
                   
  double bestStepSize = 0.0; 
  double bestSimilarity = std::numeric_limits<double>::max(); 
                        
  // Optimise the step size to give the best similarity. 
  if (this->m_CheckSimilarityMeasure)
  {
    if (this->m_FluidVelocityToDeformationFilter->GetMaxDeformation() > std::numeric_limits<float>::min())
    {
      double bestTimeStep = ComputeBestStepSize(current, next, currentFixed, nextFixed, currentSimilarity, &bestSimilarity); 
      bestStepSize = bestTimeStep/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation();
      bestFixedImageStepSize = bestTimeStep/this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation();
    }
  }
  else
  {
    // New simple adaptive size gradient descent. 
    EstimateSimpleAdapativeStepSize(&bestStepSize, &bestFixedImageStepSize); 
    
    // Not sure about this. 
    // EstimateAdapativeBarzilaiBorweinStepSize(&bestStepSize, &bestFixedImageStepSize); 
  }
  
  this->m_StepSizeHistoryForMovingImage[this->m_CurrentIteration] = bestStepSize; 
  this->m_StepSizeHistoryForFixedImage[this->m_CurrentIteration] = bestFixedImageStepSize; 
  
  // Now marshall vectors into derivative array.
  OutputImagePointer output = m_FluidVelocityToDeformationFilter->GetOutput();
  OutputImageIteratorType iterator(output, output->GetLargestPossibleRegion());
  
  // Compare this with FFDGradientDescentOptimizer.
  // For BSpline, we add a regular step size to each grid point.
  // For Fluid, the deformation is already the output of the grid. 
    
  typedef ImageRegionIterator<typename DeformableTransformType::DeformableParameterType> DeformableParametersIteratorType; 
  DeformableParametersIteratorType currentDeformableParametersIterator(current, current->GetLargestPossibleRegion()); 
  DeformableParametersIteratorType nextDeformableParametersIterator(next, next->GetLargestPossibleRegion()); 
  
  for (currentDeformableParametersIterator.GoToBegin(), nextDeformableParametersIterator.GoToBegin(), iterator.GoToBegin(); 
       !iterator.IsAtEnd(); 
       ++currentDeformableParametersIterator, ++nextDeformableParametersIterator, ++iterator)
  {
    nextDeformableParametersIterator.Set(currentDeformableParametersIterator.Get() + iterator.Get()*bestStepSize); 
  }
  
  if (this->m_IsSymmetric)
  {
    OutputImageIteratorType fixedIterator(m_FluidVelocityToFixedImageDeformationFilter->GetOutput(), m_FluidVelocityToFixedImageDeformationFilter->GetOutput()->GetLargestPossibleRegion());
    DeformableParametersIteratorType currentFixedDeformableParametersIterator(currentFixed, currentFixed->GetLargestPossibleRegion()); 
    DeformableParametersIteratorType nextFixedDeformableParametersIterator(nextFixed, nextFixed->GetLargestPossibleRegion()); 
    
    for (currentFixedDeformableParametersIterator.GoToBegin(), nextFixedDeformableParametersIterator.GoToBegin(), fixedIterator.GoToBegin(); 
        !fixedIterator.IsAtEnd(); 
        ++currentFixedDeformableParametersIterator, ++nextFixedDeformableParametersIterator, ++fixedIterator)
    {
      nextFixedDeformableParametersIterator.Set(currentFixedDeformableParametersIterator.Get() + fixedIterator.Get()*bestFixedImageStepSize); 
    }
  }
    
  niftkitkInfoMacro(<< "CalculateNextStep():Best similarity=" << bestSimilarity);
  // Forcing the step size to 0 if we cannot find any better step size. 
  if (bestStepSize == 0.0)
  {
    this->m_StepSize = 0.0; 
    bestSimilarity = std::numeric_limits<double>::max(); 
  }
  this->m_PreviousGradient = this->m_FluidVelocityToDeformationFilter->GetOutput(); 
  this->m_PreviousGradient->DisconnectPipeline(); 
  if (this->m_IsSymmetric)
  {
    this->m_PreviousFixedImageGradient = this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput(); 
    this->m_PreviousFixedImageGradient->DisconnectPipeline(); 
  }
    
  niftkitkInfoMacro(<< "CalculateNextStep():Finished");
  
  return bestSimilarity; 
}

template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
double
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::ComputeSimilarityMeasure(typename DeformableTransformType::DeformableParameterPointerType current, 
                           typename DeformableTransformType::DeformableParameterPointerType next, 
                           typename DeformableTransformType::DeformableParameterPointerType currentFixed, 
                           typename DeformableTransformType::DeformableParameterPointerType nextFixed,                           
                           double stepSize)
{
  niftkitkDebugMacro(<< "ComputeSimilarityMeasure: started");
  
  // Evaluate the similarity of the current step size. 
  ParametersType dummyParameters; 
  double maxDeformation = this->m_FluidVelocityToDeformationFilter->GetMaxDeformation(); 
  OutputImagePointer output = this->m_FluidVelocityToDeformationFilter->GetOutput();
  OutputImageIteratorType iterator(output, output->GetLargestPossibleRegion());
  typedef ImageRegionIterator<typename DeformableTransformType::DeformableParameterType> DeformableParametersIteratorType; 
  DeformableParametersIteratorType currentDeformableParametersIterator(current, current->GetLargestPossibleRegion()); 
  DeformableParametersIteratorType nextDeformableParametersIterator(next, next->GetLargestPossibleRegion()); 
  
  for (currentDeformableParametersIterator.GoToBegin(), nextDeformableParametersIterator.GoToBegin(), iterator.GoToBegin(); 
       !iterator.IsAtEnd(); 
       ++currentDeformableParametersIterator, ++nextDeformableParametersIterator, ++iterator)
  {
    nextDeformableParametersIterator.Set(currentDeformableParametersIterator.Get() + iterator.Get()*(stepSize/maxDeformation)); 
  }
  GetFluidDeformableTransform()->SetDeformableParameters(next); 
  
  if (this->m_IsSymmetric)
  {
    niftkitkDebugMacro(<< "ComputeSimilarityMeasure: try to update the fixed transform");
    double maxForwardDeformation = this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation(); 
    niftkitkDebugMacro(<< "ComputeSimilarityMeasure: maxForwardDeformation=" << maxForwardDeformation);
    OutputImagePointer forwardOutput = this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput();
    OutputImageIteratorType fowardIterator(forwardOutput, forwardOutput->GetLargestPossibleRegion());
    DeformableParametersIteratorType currentFixedDeformableParametersIterator(currentFixed, currentFixed->GetLargestPossibleRegion()); 
    DeformableParametersIteratorType nextFixedDeformableParametersIterator(nextFixed, nextFixed->GetLargestPossibleRegion()); 
  
    niftkitkDebugMacro(<< "ComputeSimilarityMeasure: loop");
    for (currentFixedDeformableParametersIterator.GoToBegin(), nextFixedDeformableParametersIterator.GoToBegin(), fowardIterator.GoToBegin(); 
        !currentFixedDeformableParametersIterator.IsAtEnd(); 
        ++currentFixedDeformableParametersIterator, ++nextFixedDeformableParametersIterator, ++fowardIterator)
    {
      nextFixedDeformableParametersIterator.Set(currentFixedDeformableParametersIterator.Get() + fowardIterator.Get()*(stepSize/maxForwardDeformation)); 
    }
    this->m_FixedImageTransform->SetDeformableParameters(nextFixed); 
  }
  
  niftkitkDebugMacro(<< "ComputeSimilarityMeasure: trying to get similarity measure");
  return (this->m_Maximize?-1.0:1.0)*this->m_ImageToImageMetric->GetValue(dummyParameters);
}

template <class TFixedImage, class TMovingImage, class TScalar, class TDeformationScalar>
double
FluidGradientDescentOptimizer< TFixedImage, TMovingImage, TScalar, TDeformationScalar>
::ComputeBestStepSize(typename DeformableTransformType::DeformableParameterPointerType current, typename DeformableTransformType::DeformableParameterPointerType next, typename DeformableTransformType::DeformableParameterPointerType currentFixed, typename DeformableTransformType::DeformableParameterPointerType nextFixed, double currentSimilarity, double* bestSimilarity)
{
  niftkitkDebugMacro(<< "ComputeBestStepSize: started");
  double bestStep = 0.0; 
  
  if (currentSimilarity == std::numeric_limits<double>::max())
  {
    currentSimilarity = ComputeSimilarityMeasure(current, next, currentFixed, nextFixed, bestStep); 
    niftkitkInfoMacro(<< "Check similarity: currentSimilarity1=" << currentSimilarity); 
  }
  else
  {
    currentSimilarity = (this->m_Maximize?-1.0:1.0)*currentSimilarity; 
    niftkitkInfoMacro(<< "Check similarity: currentSimilarity2=" << currentSimilarity); 
  }
  niftkitkInfoMacro(<< "Check similarity: currentSimilarity3=" << currentSimilarity); 
  
  bool isBetterSimilarity = false; 
  // while (this->m_StepSize >= this->m_MinimumDeformationAllowedForIterations && !isBetterSimilarity)
  {
    double nextSimilarity = ComputeSimilarityMeasure(current, next, currentFixed, nextFixed, this->m_StepSize); 
    niftkitkInfoMacro(<< "Check similarity: nextSimilarity=" << nextSimilarity); 
    *bestSimilarity = (this->m_Maximize?-1.0:1.0)*nextSimilarity; 
    
    double fieldDotProduct = -(nextSimilarity-currentSimilarity); 
    const double f_max = this->m_AsgdFMax; 
    const double f_min = this->m_AsgdFMin; 
    const double w = this->m_AsgdW; 
    m_AdjustedTimeStep = m_AdjustedTimeStep + f_min + (f_max-f_min)/(1.-(f_max/f_min)*exp(-(-fieldDotProduct)/w)); 
    niftkitkInfoMacro(<< "Check similarity: m_AdjustedTimeStep=" << m_AdjustedTimeStep << ",f_max=" << f_max << ",f_min=" << f_min << ",w=" << w); 
    m_AdjustedTimeStep = std::max<double>(0., m_AdjustedTimeStep); 
    this->m_StepSize = this->m_StartingStepSize/(m_AdjustedTimeStep+1.); 
    niftkitkInfoMacro(<< "Check similarity: this->m_StepSize=" << this->m_StepSize); 
    
    if (fieldDotProduct > 0.)  
      isBetterSimilarity = true; 
    else
      isBetterSimilarity = false; 
  }
  
  if (this->m_StepSize < this->m_MinimumDeformationAllowedForIterations)
  {
    this->m_CurrentMinimumDeformationIterations++; 
    niftkitkDebugMacro(<< "m_CurrentMinimumDeformationIterations=" << this->m_CurrentMinimumDeformationIterations);
  }
  if (this->m_MinimumDeformationMaximumIterations > -1 && this->m_CurrentMinimumDeformationIterations > this->m_MinimumDeformationMaximumIterations)
  {
    this->m_StepSize = 0.0; 
    niftkitkDebugMacro(<< "m_CurrentMinimumDeformationIterations=" << this->m_CurrentMinimumDeformationIterations);
    niftkitkDebugMacro(<< "m_MinimumDeformationMaximumIterations=" << this->m_MinimumDeformationMaximumIterations);
    niftkitkDebugMacro(<< "Setting best step size to 0");
  }
  
  return this->m_StepSize;   
}


template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
void
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::ResumeOptimization(void)
{
  niftkitkInfoMacro(<< "ResumeOptimization():Started");
  ParametersType dummyParameters; 
  double stepSize = this->m_StepSize;
  double minJacobian = 0;
  double minFixedJacobian = 0.; 
  
  this->m_CurrentIteration = 0;

  // ReGrid first of all.
  this->ReGrid(false);
  
  niftkitkInfoMacro(<< "ResumeOptimization():Starting with initial value of:" << niftk::ConvertToString(this->m_Value));
  
  // Stop can be set by another thread WHILE we are iterating (potentially).
  this->m_Stop = false;
  
  // Subclasses can initialize at this point
  this->Initialize();
  
  double nextValue = std::numeric_limits<double>::max(); 
  
  while( !this->m_Stop ) 
  {
    this->m_CurrentIteration++;
    
    double maxDeformationChange = 0.0;
  
    if (this->m_CurrentIteration > this->m_MaximumNumberOfIterations)
      {
        niftkitkInfoMacro(<< "ResumeOptimization():Hit max iterations:" << this->m_MaximumNumberOfIterations);
        this->StopOptimization();
        break;
      }
    
    if (this->m_StepSize < this->m_MinimumStepSize)
      {
        niftkitkInfoMacro(<< "ResumeOptimization():Gone below min step size:" << this->m_MinimumStepSize);
        this->StopOptimization();
        break;
      }

    // Reset the next position array.
    // this->m_NextDeformableParameters->FillBuffer(static_cast<TDeformationScalar>(0)); 
  
    // Calculate the next set of parameters, which may or may not be 'accepted'.
    nextValue = CalculateNextStep(this->m_CurrentIteration, this->m_Value, this->m_CurrentDeformableParameters, this->m_NextDeformableParameters, this->m_CurrentFixedDeformableParameters, this->m_NextFixedDeformableParameters);
    
    // Set them onto the transform. We dont need this, as computing the similarity measure sets the transform.
    // this->m_DeformableTransform->SetParameters(this->m_NextParameters);

    // So far so good, so compute new similarity measure.                                                                               
    if (nextValue == std::numeric_limits<double>::max())
    {
      GetFluidDeformableTransform()->SetDeformableParameters(this->m_NextDeformableParameters);
      if (this->m_IsSymmetric)
      {
        this->m_FixedImageTransform->SetDeformableParameters(this->m_NextFixedDeformableParameters); 
      }
      nextValue = this->GetCostFunction()->GetValue(dummyParameters);
      
#if 0      
      if (this->m_IsSymmetric)
      {
        typedef ImageFileWriter<TFixedImage> WriterType; 
        typename WriterType::Pointer writer = WriterType::New(); 
        char name[255]; 
        writer->SetInput(this->m_ImageToImageMetric->GetTransformedFixedImage()); 
        sprintf(name, "fixed_transformed_%04d.img", this->m_CurrentIteration); 
        writer->SetFileName(name); 
        writer->Update(); 
        writer->SetInput(this->m_ImageToImageMetric->GetTransformedMovingImage()); 
        sprintf(name, "moving_transformed_%04d.img", this->m_CurrentIteration); 
        writer->SetFileName(name); 
        writer->Update(); 
      }
#endif          
    }
    else
    {
      // Need this for checking Jacobian. 
      GetFluidDeformableTransform()->SetDeformableParameters(this->m_NextDeformableParameters);
    }

    // Write the parameters to file.
    if (this->m_WriteNextParameters)
      {
        this->m_DeformableTransform->WriteParameters(this->m_NextParametersFileName + "." + niftk::ConvertToString((int)this->m_CurrentIteration) + "." + this->m_NextParametersFileExt);
      }

    // Write this deformation field to file. 
    // For Fluid, this is the same as the dump of the parameters above.
    // For FFD, the one above will be the control point grid, so this is the actual deformation field.
    if (this->m_WriteDeformationField)
      {
        this->m_DeformableTransform->WriteTransform(this->m_DeformationFieldFileName + "." + niftk::ConvertToString((int)this->m_CurrentIteration) + "." + this->m_DeformationFieldFileExt);    
      }
    
    minJacobian = this->m_DeformableTransform->ComputeMinJacobian();
    minFixedJacobian = this->m_FixedImageTransform->ComputeMinJacobian(); 
    niftkitkInfoMacro(<< "ResumeOptimization():minJacobian=" << minJacobian << ",minFixedJacobian=" << minFixedJacobian); 
    minJacobian = std::min<double>(minJacobian, minFixedJacobian); 

    if (this->m_CheckJacobianBelowZero && minJacobian < 0.0)
      {
        this->m_StepSize *= this->m_JacobianBelowZeroStepSizeReductionFactor;
        
        niftkitkInfoMacro(<< "ResumeOptimization():Iteration=" << this->m_CurrentIteration \
          << ", Negative Jacobian " << minJacobian << ", so rejecting this change, reducing step size by:" \
          << this->m_JacobianBelowZeroStepSizeReductionFactor \
          << ", to:" << this->m_StepSize \
          << ", and continuing");
      }
    else
      {
        niftkitkInfoMacro(<< "ResumeOptimization():Iteration=" << this->m_CurrentIteration \
          << ", currentValue=" << niftk::ConvertToString(this->m_Value) \
          << ", nextValue=" << niftk::ConvertToString(nextValue) \
          << ", minJacobian=" <<  niftk::ConvertToString(minJacobian) \
          << ", maxDeformationChange=" <<  niftk::ConvertToString(maxDeformationChange));
      
        if (true)
          /*(this->m_Maximize && nextValue > this->m_Value)
           || (!this->m_Maximize && nextValue < this->m_Value) 
           || !this->m_CheckSimilarityMeasure)*/
          {

            niftkitkInfoMacro(<< "ResumeOptimization():Maximize:" << this->m_Maximize \
              << ", currentValue:" << niftk::ConvertToString(this->m_Value) \
              << ", nextValue:" << niftk::ConvertToString(nextValue) \
              << ", so its better!");
          
            if (fabs(nextValue - this->m_Value) <  this->m_MinimumSimilarityChangeThreshold
                && this->m_CheckSimilarityMeasure)
              {
                niftkitkInfoMacro(<< "ResumeOptimization():Iteration=" << this->m_CurrentIteration \
                  << ", similarity change was:" << fabs(nextValue - this->m_Value) \
                  << ", threshold was:" << this->m_MinimumSimilarityChangeThreshold \
                  << ", so I'm accepting this change, but then stopping.");
                this->m_CurrentDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_NextDeformableParameters); 
                if (this->m_IsSymmetric)
                {
                  this->m_CurrentFixedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_NextFixedDeformableParameters); 
                }
                this->m_Value = nextValue;
                this->StopOptimization();
                break;
              }
            
            bool isAcceptNextParameterAccordingJacobian = true; 
            // Check if we need to regrid only after we got a better similarity value. 
            if (minJacobian < this->m_MinimumJacobianThreshold)
              {
                if (this->m_DeformableTransform->IsRegridable())
                  {
                    niftkitkInfoMacro(<< "ResumeOptimization():Jacobian is lower than threshold:" <<  this->m_MinimumJacobianThreshold \
                      << ", so accepting this change, reducing step size by:" \
                      << this->m_RegriddingStepSizeReductionFactor \
                      << ", to:" << this->m_StepSize \
                      << ", regridding and continuing");

                    this->m_StepSize *= this->m_RegriddingStepSizeReductionFactor;

                    this->ReGrid(true);
                    isAcceptNextParameterAccordingJacobian = false; 
                  }
                else
                  {
                    niftkitkInfoMacro(<< "ResumeOptimization():Jacobian is lower than threshold:" <<  this->m_MinimumJacobianThreshold \
                      << ", but this->m_DeformableTransform->IsRegridable() is false, so I'm not going to regrid.");
                  }
              } 
            
            if (isAcceptNextParameterAccordingJacobian)
            {
              this->m_CurrentDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_NextDeformableParameters); 
              if (this->m_IsSymmetric)
              {
                this->m_CurrentFixedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_NextFixedDeformableParameters); 
              }
              this->m_Value = nextValue;
            }
            
          }
        else
          {
            // Revert the transformed moving image to the current parameter. 
            // We already have the current value, no need to re-evaluate it.
            // this->GetCostFunction()->GetValue(this->GetCurrentPosition());
            
            this->m_StepSize *= this->m_IteratingStepSizeReductionFactor;

            niftkitkInfoMacro(<< "ResumeOptimization():Maximize:" << this->m_Maximize \
              << ", currentValue:" << niftk::ConvertToString(this->m_Value) \
              << ", nextValue:" << niftk::ConvertToString(nextValue) \
              << ", so its no better, so rejecting this change, reducing step size by:" \
              << this->m_IteratingStepSizeReductionFactor \
              << ", to:" << this->m_StepSize \
              << " and continuing");
          }
      } 

  } // End main while loop.

  // Subclasses can cleanup at this point
  this->CleanUp();
  
  niftkitkInfoMacro(<< "ResumeOptimization():Aggregating transformation");
  
  // Add deformation field to existing regridding one.  
  if (this->m_DeformableTransform->IsRegridable())
    {
      this->ComposeJacobian(); 
      
      if (this->m_IsPropagateRegriddedMovingImage)
      {
        niftkitkInfoMacro(<< "ResumeOptimization(): propagate regridded moving image.");
        this->m_RegriddingResampler->SetTransform(this->m_DeformableTransform);
        this->m_RegriddingResampler->SetInterpolator(this->m_RegriddingInterpolator);      
        this->m_RegriddingResampler->SetInput(this->m_RegriddedMovingImage);
        this->m_RegriddingResampler->SetOutputParametersFromImage(this->m_FixedImage);
        this->m_RegriddingResampler->SetDefaultPixelValue(this->m_RegriddedMovingImagePadValue);
        this->m_RegriddingResampler->Modified();  
        this->m_RegriddingResampler->UpdateLargestPossibleRegion();
        this->m_RegriddedMovingImage = this->m_RegriddingResampler->GetOutput();
        this->m_RegriddedMovingImage->DisconnectPipeline();
        
        if (this->m_IsSymmetric)
        {
          niftkitkInfoMacro(<< "ResumeOptimization(): propagate regridded fixed image.");
          this->m_RegriddingFixedImageResampler->SetTransform(this->m_FixedImageTransform);
          this->m_RegriddingFixedImageResampler->SetInterpolator(this->m_RegriddingInterpolator);      
          this->m_RegriddingFixedImageResampler->SetInput(this->m_RegriddedFixedImage);
          this->m_RegriddingFixedImageResampler->SetOutputParametersFromImage(this->m_FixedImage);
          this->m_RegriddingFixedImageResampler->SetDefaultPixelValue(this->m_RegriddedMovingImagePadValue);
          this->m_RegriddingFixedImageResampler->Modified();  
          this->m_RegriddingFixedImageResampler->UpdateLargestPossibleRegion();
          this->m_RegriddedFixedImage = this->m_RegriddingFixedImageResampler->GetOutput();
          this->m_RegriddedFixedImage->DisconnectPipeline();
        }
      }
      
      GetFluidDeformableTransform()->UpdateRegriddedDeformationParameters(this->m_RegriddedDeformableParameters, this->m_CurrentDeformableParameters, 1); 
      GetFluidDeformableTransform()->SetDeformableParameters(this->m_RegriddedDeformableParameters); 
      
      if (this->m_IsSymmetric)
      {
        this->m_FixedImageTransform->UpdateRegriddedDeformationParameters(this->m_RegriddedFixedDeformableParameters, this->m_CurrentFixedDeformableParameters, 1); 
        this->m_FixedImageTransform->SetDeformableParameters(this->m_RegriddedFixedDeformableParameters); 
      }
      
    }

  // Im resetting the m_Step size, so that if you repeatedly call 
  // the optimizer, it does actually do some optimizing.
  this->m_StepSize = stepSize;
  
  niftkitkInfoMacro(<< "ResumeOptimization():Finished");
}

#if 1
template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
void
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::ReGrid(bool isResetCurrentPosition)
{
  niftkitkInfoMacro(<< "ReGrid():Started");
  
  if (this->m_CurrentIteration == 0)
  {
    if (this->m_DeformableTransform->GetGlobalTransform() == NULL)
    {
      niftkitkInfoMacro(<< "ReGrid():First iteration, set all to 0");
      this->m_RegriddedDeformableParameters->FillBuffer(static_cast<TDeformationScalar>(0)); 
      if (this->m_IsSymmetric)
      {
        this->m_RegriddedFixedDeformableParameters->FillBuffer(static_cast<TDeformationScalar>(0)); 
      }
    }
    else
    {
        niftkitkInfoMacro(<< "ReGrid():First iteration, setting global transform.");
        this->m_DeformableTransform->InitialiseGlobalTransformation();
        this->m_RegriddedDeformableParameters = DeformableTransformType::DuplicateDeformableParameters(GetFluidDeformableTransform()->GetDeformableParameters());
    }
  }
  
  if (isResetCurrentPosition)
  {
    niftkitkInfoMacro(<< "ReGrid():Adding current array to regrid array");
    typename FluidTransformType::DeformableParameterType::Pointer currentParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentDeformableParameters); 
    GetFluidDeformableTransform()->UpdateRegriddedDeformationParameters(this->m_RegriddedDeformableParameters, currentParameters, 1); 
    
    if (this->m_IsSymmetric)
    {
      typename FluidTransformType::DeformableParameterType::Pointer currentParameters = DeformableTransformType::DuplicateDeformableParameters(this->m_CurrentFixedDeformableParameters); 
      this->m_FixedImageTransform->UpdateRegriddedDeformationParameters(this->m_RegriddedFixedDeformableParameters, currentParameters, 1); 
    }
  }

  // Set transform.
  if (this->m_CurrentIteration == 0)
  {
    GetFluidDeformableTransform()->SetDeformableParameters(this->m_RegriddedDeformableParameters);
    if (this->m_IsSymmetric)
    {
      this->m_FixedImageTransform->SetDeformableParameters(this->m_RegriddedFixedDeformableParameters);
    }
  }
  else
  {
    if (!this->m_IsPropagateRegriddedMovingImage)
    {
      GetFluidDeformableTransform()->SetDeformableParameters(this->m_RegriddedDeformableParameters);
    }
    else
    {
      niftkitkInfoMacro(<< "ReGrid():setting current position for propagating moving image.");
      GetFluidDeformableTransform()->SetDeformableParameters(this->m_CurrentDeformableParameters);
      if (this->m_IsSymmetric)
      {
        this->m_FixedImageTransform->SetDeformableParameters(this->m_CurrentFixedDeformableParameters);
      }
    }
  }
  // Compose the Jacobians. 
  this->ComposeJacobian(); 

//  #ifdef(DEBUG)
  {
    niftkitkInfoMacro(<< "ReGrid():Check before resample, minJacobian=" \
    <<  this->m_DeformableTransform->ComputeMinJacobian() \
    << ", maxJacobian=" << this->m_DeformableTransform->ComputeMaxJacobian() \
    << ", minDeformation=" << this->m_DeformableTransform->ComputeMinDeformation() \
    << ", maxDeformation=" << this->m_DeformableTransform->ComputeMaxDeformation() );
  }
  
  // Transform the original moving image. So, if isResetCurrentPosition we assume
  // that this->m_DeformableTransform has an up to date transformation.
  if (this->m_DeformableTransform->IsIdentity())
    {
      niftkitkInfoMacro(<< "ReGrid():Regridding using identity transform, and nearest neighbour interpolator, as its quicker than using m_DeformableTransform");
      typedef IdentityTransform<double, Dimension> IdentityTransformType;
      typename IdentityTransformType::Pointer identityTransform = IdentityTransformType::New();
      
      typedef NearestNeighborInterpolateImageFunction<TFixedImage, double> NearestInterpolatorType;
      typename NearestInterpolatorType::Pointer nearestInterpolator = NearestInterpolatorType::New();
      
      this->m_RegriddingResampler->SetTransform(identityTransform);
      this->m_RegriddingResampler->SetInterpolator(nearestInterpolator);
      if (this->m_IsSymmetric)
      {
        this->m_RegriddingFixedImageResampler->SetTransform(identityTransform);
        this->m_RegriddingFixedImageResampler->SetInterpolator(nearestInterpolator);
      }
    }
  else
  {
    this->m_RegriddingResampler->SetTransform(this->m_DeformableTransform);
    this->m_RegriddingResampler->SetInterpolator(this->m_RegriddingInterpolator);      
    if (this->m_IsSymmetric)
    {
      this->m_RegriddingFixedImageResampler->SetTransform(this->m_FixedImageTransform);
      this->m_RegriddingFixedImageResampler->SetInterpolator(this->m_RegriddingInterpolator);
    }
  }

  if (this->m_CurrentIteration == 0)
  {
    this->m_RegriddingResampler->SetInput(this->m_MovingImage);
    if (this->m_IsSymmetric)
    {
      this->m_RegriddingFixedImageResampler->SetInput(this->m_FixedImage); 
    }
  }
  else
  {
    if (!this->m_IsPropagateRegriddedMovingImage)
    {
      this->m_RegriddingResampler->SetInput(this->m_MovingImage);
      if (this->m_IsSymmetric)
      {
        this->m_RegriddingFixedImageResampler->SetInput(this->m_FixedImage); 
      }
    }
    else
    {
      this->m_RegriddingResampler->SetInput(this->m_RegriddedMovingImage);
      if (this->m_IsSymmetric)
      {
        this->m_RegriddingFixedImageResampler->SetInput(this->m_RegriddedFixedImage);
      } 
    }
  }
  this->m_RegriddingResampler->SetOutputParametersFromImage(this->m_FixedImage);
  this->m_RegriddingResampler->SetDefaultPixelValue(this->m_RegriddedMovingImagePadValue);
  this->m_RegriddingResampler->Modified();  
  this->m_RegriddingResampler->UpdateLargestPossibleRegion();
  this->m_RegriddedMovingImage = this->m_RegriddingResampler->GetOutput();
  this->m_RegriddedMovingImage->DisconnectPipeline();
  
  if (this->m_IsSymmetric)
  {
    this->m_RegriddingFixedImageResampler->SetOutputParametersFromImage(this->m_FixedImage);
    this->m_RegriddingFixedImageResampler->SetDefaultPixelValue(this->m_RegriddedMovingImagePadValue);
    this->m_RegriddingFixedImageResampler->Modified();  
    this->m_RegriddingFixedImageResampler->UpdateLargestPossibleRegion();
    this->m_RegriddedFixedImage = this->m_RegriddingFixedImageResampler->GetOutput();
    this->m_RegriddedFixedImage->DisconnectPipeline();
  }  
  
  if (this->m_IsAbsRegriddedImage)
  {
    typedef AbsImageFilter<TFixedImage, TFixedImage> AbsImageFilterType; 
    typename AbsImageFilterType::Pointer absImageFilter = AbsImageFilterType::New(); 
    
    absImageFilter->SetInput(this->m_RegriddedMovingImage); 
    absImageFilter->Update(); 
    this->m_RegriddedMovingImage = absImageFilter->GetOutput(); 
    this->m_RegriddedMovingImage->DisconnectPipeline();
    
    if (this->m_IsSymmetric)
    {
      absImageFilter->SetInput(this->m_RegriddedFixedImage); 
      absImageFilter->Update(); 
      this->m_RegriddedFixedImage = absImageFilter->GetOutput(); 
      this->m_RegriddedFixedImage->DisconnectPipeline();
    }    
  }

  niftkitkInfoMacro(<< "ReGrid():m_RegriddedMovingImage is currently at address=" << &this->m_RegriddedMovingImage \
    << ", size=" << this->m_RegriddedMovingImage->GetLargestPossibleRegion().GetSize() \
    << ", spacing=" << this->m_RegriddedMovingImage->GetSpacing()
    << ", origin=" << this->m_RegriddedMovingImage->GetOrigin() \
    << ", direction=\n" << this->m_RegriddedMovingImage->GetDirection());

  // Reset the transform, set transform to Identity, 
  // as its quicker than reinterpolating a BSpline grid.
  if (isResetCurrentPosition)
  {
    this->m_CurrentDeformableParameters->FillBuffer(static_cast<TDeformationScalar>(0)); 
    if (this->m_IsSymmetric)
    {
      this->m_CurrentFixedDeformableParameters->FillBuffer(static_cast<TDeformationScalar>(0)); 
    }
  }

  // Recompute the similarity.
  (const_cast<typename Superclass::ImageToImageMetricType*>(this->m_ImageToImageMetric))->SetMovingImage(this->m_RegriddedMovingImage);
  if (this->m_IsSymmetric)
  {
    (const_cast<typename Superclass::ImageToImageMetricType*>(this->m_ImageToImageMetric))->SetFixedImage(this->m_RegriddedFixedImage);
  }
  
  GetFluidDeformableTransform()->SetDeformableParameters(this->m_CurrentDeformableParameters);
  if (this->m_IsSymmetric)
  {
    this->m_FixedImageTransform->SetDeformableParameters(this->m_CurrentFixedDeformableParameters);
  }
  
  // Reintitialise the bins. 
  (const_cast<typename Superclass::ImageToImageMetricType*>(this->m_ImageToImageMetric))->Initialize();
  
  this->m_Value = this->m_ImageToImageMetric->GetValue(this->m_DeformableTransform->GetParameters());
  
  niftkitkInfoMacro(<< "ReGrid():Setting current value to:" << this->m_Value);

 // #ifdef(DEBUG)
  {
    niftkitkInfoMacro(<< "ReGrid():Check after resetting to current position, minJacobian=" \
      <<  this->m_DeformableTransform->ComputeMinJacobian() \
      << ", maxJacobian=" << this->m_DeformableTransform->ComputeMaxJacobian() \
      << ", minDeformation=" << this->m_DeformableTransform->ComputeMinDeformation() \
      << ", maxDeformation=" << this->m_DeformableTransform->ComputeMaxDeformation() );
  }

  this->m_NormaliseStepSize = true; 
  
  if (isResetCurrentPosition)
    this->m_CalculateVelocityFeild = false; 
  
  niftkitkDebugMacro(<< "ReGrid():Finished.");

}
#endif


/**
 * Estimate the variance of the error/noise in the gradient/velocity measurement by random sampling of the images. 
 */
template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
double 
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::EstimateGradientErrorVariance(int numberOfSamples, int numberOfSimulations, typename FluidPDEType::OutputImageType::Pointer originalVelocity, double* eta)
{
  std::cerr << "EstimateGradientErrorVariance: begins" << std::endl; 
  
  AsgdMaskIteratorType iterator(this->m_AsgdMask, this->m_AsgdMask->GetLargestPossibleRegion());
  typename MaskType::RegionType boundingBox; 
  typename MaskType::IndexType bottomLeftIndex; 
  typename MaskType::IndexType topRightIndex; 
  int numberOfVoxels = 0; 
  
  // Recover the boundary box of the ASGD mask. 
  bottomLeftIndex.Fill(std::numeric_limits<int>::max()); 
  topRightIndex.Fill(0); 
  for (iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator)
  {
    if (iterator.Get() > 128) 
    {
      for (unsigned int i = 0; i < Dimension; i++)
      {
        if (iterator.GetIndex()[i] < bottomLeftIndex[i])
        {
          bottomLeftIndex[i] = iterator.GetIndex()[i]; 
        }
        if (iterator.GetIndex()[i] > topRightIndex[i])
        {
          topRightIndex[i] = iterator.GetIndex()[i]; 
        }
      }
      numberOfVoxels++; 
    }
  }
  
  typename MaskType::SizeType size; 
  for (unsigned int i = 0; i < Dimension; i++)
  {
    size[i] = topRightIndex[i] - bottomLeftIndex[i] + 1; 
  }
  boundingBox.SetIndex(bottomLeftIndex); 
  boundingBox.SetSize(size); 
  std::cerr << "bottomLeftIndex=" << bottomLeftIndex << ",topRightIndex=" << topRightIndex << ", boundingBox=" << boundingBox << ", numberOfVoxels=" << numberOfVoxels << std::endl; 
  
  // Randomly sample the fixed and moving image to calculate the gradient. 
  double totalSumOfSquare = 0.; 
  double sumOfVelocityField = 0.; 
  typename FluidPDEType::OutputImageType::Pointer velocityErrors; 
  typedef ImageDuplicator<TFixedImage> ImageDuplicatorType; 
  for (int i = 0; i < numberOfSimulations; i++)
  {
    typename ImageDuplicatorType::Pointer randomSampledFixedImage = ImageDuplicatorType::New(); 
    typename ImageDuplicatorType::Pointer randomSampledMovingImage = ImageDuplicatorType::New(); 
    
    randomSampledFixedImage->SetInputImage(this->m_ImageToImageMetric->GetTransformedFixedImage()); 
    randomSampledFixedImage->Update(); 
    randomSampledFixedImage->GetOutput()->FillBuffer(0.f); 
    randomSampledMovingImage->SetInputImage(this->m_ImageToImageMetric->GetTransformedMovingImage()); 
    randomSampledMovingImage->Update(); 
    randomSampledMovingImage->GetOutput()->FillBuffer(0.f);

    ImageRandomNonRepeatingIteratorWithIndex<MaskType> randomIterator(this->m_AsgdMask, boundingBox); 
    randomIterator.SetNumberOfSamples(numberOfSamples); 
    for (randomIterator.GoToBegin(); !randomIterator.IsAtEnd(); ++randomIterator)
    {
      typename MaskType::IndexType index = randomIterator.GetIndex(); 
      randomSampledFixedImage->GetOutput()->SetPixel(index, this->m_ImageToImageMetric->GetTransformedFixedImage()->GetPixel(index)); 
      randomSampledMovingImage->GetOutput()->SetPixel(index, this->m_ImageToImageMetric->GetTransformedMovingImage()->GetPixel(index)); 
    }
  
    m_ForceFilter->SetFixedImage(randomSampledFixedImage->GetOutput());
    m_ForceFilter->SetIsSymmetric(this->m_IsSymmetric); 
    m_ForceFilter->SetTransformedMovingImage(randomSampledMovingImage->GetOutput());
    m_ForceFilter->SetUnTransformedMovingImage(randomSampledMovingImage->GetOutput());
    m_ForceFilter->SetFixedImageMask(this->m_ImageToImageMetric->GetFixedImageMask()); 
    m_ForceFilter->Modified();
    m_ForceFilter->UpdateLargestPossibleRegion();
    m_FluidPDESolver->SetInput(this->m_ForceFilter->GetOutput());
    m_FluidPDESolver->UpdateLargestPossibleRegion();
    typename FluidPDEType::OutputImageType::Pointer velocityField = m_FluidPDESolver->GetOutput(); 
    velocityField->DisconnectPipeline(); 
    
    // Calculate the variance. 
    AsgdMaskIteratorType iterator(this->m_AsgdMask, this->m_AsgdMask->GetLargestPossibleRegion());
    OutputImageIteratorType originalIterator(originalVelocity, originalVelocity->GetLargestPossibleRegion()); 
    OutputImageIteratorType approximateIterator(velocityField, velocityField->GetLargestPossibleRegion()); 
    
    double currentSumOfSquare = 0.; 
    for (iterator.GoToBegin(), originalIterator.GoToBegin(), approximateIterator.GoToBegin(); 
         !iterator.IsAtEnd(); 
         ++iterator, ++originalIterator, ++approximateIterator)
    {
      if (iterator.Get() > 128)
      {
        currentSumOfSquare += (originalIterator.Get()-approximateIterator.Get()).GetSquaredNorm(); 
        if (i == 0)
        {
          sumOfVelocityField += originalIterator.Get().GetSquaredNorm(); 
        }
      }
    }
    totalSumOfSquare += currentSumOfSquare; 
    
    std::cerr << "currentSumOfSquare=" << currentSumOfSquare << std::endl; 
  }
  
  // 2.0 for decreasing the gain a bit by increasing the noise factor. 
  // => we want a slower descent. 
  *eta = sumOfVelocityField/(sumOfVelocityField+this->m_AsgdFMinFudgeFactor*totalSumOfSquare/numberOfSimulations); 
  std::cerr << "*eta=" << *eta << std::endl; 
  
  double variance = totalSumOfSquare/(numberOfVoxels*numberOfSimulations); 
  std::cerr << "variance=" << variance << std::endl; 
  
  // Adjust it for the actual step size. 
  variance *= (this->m_StartingStepSize/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation())*(this->m_StartingStepSize/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation()); 
  
  std::cerr << "adjusted variance=" << variance << std::endl; 
  
  return variance; 
}


/**
 * Estimate step sizes using adaptive size gradient descent, see Klein (2009) Int J Comput Vis. 
 */
template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
void
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::EstimateSimpleAdapativeStepSize(double* bestStepSize, double* bestFixedImageStepSize)
{
  double fieldDotProduct = 0.; 
  OutputImageIteratorType currentGradientIterator(this->m_FluidVelocityToDeformationFilter->GetOutput(), this->m_FluidVelocityToDeformationFilter->GetOutput()->GetLargestPossibleRegion());
  AsgdMaskIteratorType* asgdMaskIterator = NULL;
  if (!this->m_AsgdMask.IsNull())
  {
    asgdMaskIterator = new AsgdMaskIteratorType(this->m_AsgdMask, this->m_AsgdMask->GetLargestPossibleRegion());
  }
  int numberOfAsgdMaskVoxels = 0; 
  if (!m_PreviousGradient.IsNull())
  {
    OutputImageIteratorType previousGradientIterator(this->m_PreviousGradient, this->m_PreviousGradient->GetLargestPossibleRegion());
    if (asgdMaskIterator != NULL)
      asgdMaskIterator->GoToBegin(); 
    for (currentGradientIterator.Begin(), previousGradientIterator.Begin(); 
        !currentGradientIterator.IsAtEnd(); 
        ++currentGradientIterator, ++previousGradientIterator)
    {
      if (asgdMaskIterator != NULL)
      {
        if (asgdMaskIterator->Get() <= 128)
        {
          ++(*asgdMaskIterator); 
          continue; 
        }
        numberOfAsgdMaskVoxels++; 
        ++(*asgdMaskIterator); 
      }
      fieldDotProduct += currentGradientIterator.Get()*previousGradientIterator.Get(); 
    }
  }
  niftkitkInfoMacro(<< "CalculateNextStep():fieldDotProduct=" << fieldDotProduct << ", m_StartingStepSize=" << this->m_StartingStepSize << ",numberOfAsgdMaskVoxels=" << numberOfAsgdMaskVoxels);
  
  double fixedImageFieldDotProduct = 0.; 
  if (this->m_IsSymmetric)
  {
    OutputImageIteratorType currentFixedGradientIterator(this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput(), this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput()->GetLargestPossibleRegion());
    if (!m_PreviousFixedImageGradient.IsNull())
    {
      numberOfAsgdMaskVoxels = 0; 
      OutputImageIteratorType previousFixedImageGradientIterator(this->m_PreviousFixedImageGradient, this->m_PreviousFixedImageGradient->GetLargestPossibleRegion());
      if (asgdMaskIterator != NULL)
        asgdMaskIterator->GoToBegin(); 
      for (currentFixedGradientIterator.Begin(), previousFixedImageGradientIterator.Begin(); 
          !currentFixedGradientIterator.IsAtEnd(); 
          ++currentFixedGradientIterator, ++previousFixedImageGradientIterator)
      {
        if (asgdMaskIterator != NULL)
        {
          if (asgdMaskIterator->Get() <= 128)
          {
            ++(*asgdMaskIterator); 
            continue; 
          }
          numberOfAsgdMaskVoxels++; 
          ++(*asgdMaskIterator); 
        }
        fixedImageFieldDotProduct += currentFixedGradientIterator.Get()*previousFixedImageGradientIterator.Get(); 
      }
    }
    niftkitkInfoMacro(<< "CalculateNextStep():fixedImageFieldDotProduct=" << fixedImageFieldDotProduct << ", m_StartingStepSize=" << this->m_StartingStepSize << ",numberOfAsgdMaskVoxels=" << numberOfAsgdMaskVoxels);
  }
  
  const double f_max = this->m_AsgdFMax; 
  const double f_min = this->m_AsgdFMin; 
  const double w = this->m_AsgdW; 
  m_AdjustedTimeStep = m_AdjustedTimeStep + f_min + (f_max-f_min)/(1.-(f_max/f_min)*exp(-(-fieldDotProduct)/w)); 
  niftkitkInfoMacro(<< "new gradient descent: m_AdjustedTimeStep=" << m_AdjustedTimeStep << ",f_max=" << f_max << ",f_min=" << f_min << ",w=" << w); 
  m_AdjustedTimeStep = std::max<double>(0., m_AdjustedTimeStep); 
  *bestStepSize = (this->m_StartingStepSize/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation())/(m_AdjustedTimeStep+this->m_AsgdA); 
  double bestDeformationChange = this->m_FluidVelocityToDeformationFilter->GetMaxDeformation()*(*bestStepSize); 
  
  if (this->m_IsSymmetric)
  {
    m_AdjustedFixedImageTimeStep = m_AdjustedFixedImageTimeStep + f_min + (f_max-f_min)/(1.-(f_max/f_min)*exp(-(-fixedImageFieldDotProduct)/w)); 
    niftkitkInfoMacro(<< "new gradient descent: m_AdjustedFixedImageTimeStep=" << m_AdjustedFixedImageTimeStep); 
    m_AdjustedFixedImageTimeStep = std::max<double>(0., m_AdjustedFixedImageTimeStep); 
    *bestFixedImageStepSize = (this->m_StartingStepSize/this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation())/(m_AdjustedFixedImageTimeStep+this->m_AsgdA); 
    
    // Symmetric step size - should move by the same amount in each direction. 
    double bestFixedDeformationChange = this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation()*(*bestFixedImageStepSize); 
    double bestSymmetricDeformationChange = std::min<double>(bestDeformationChange, bestFixedDeformationChange); 
    niftkitkInfoMacro(<< "CalculateNextStep():bestDeformationChange=" << bestDeformationChange << ",bestFixedDeformationChange=" << bestFixedDeformationChange << ",bestSymmetricDeformationChange=" << bestSymmetricDeformationChange); 
    *bestStepSize = bestSymmetricDeformationChange/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation(); 
    *bestFixedImageStepSize = bestSymmetricDeformationChange/this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation(); 
    
    bestDeformationChange = bestSymmetricDeformationChange; 
  }
  
  if (bestDeformationChange < this->m_MinimumDeformationMagnitudeThreshold)
  {
    this->m_StepSize = 0.; 
  }
  delete asgdMaskIterator; 
}


/**
 * Estimate step sizes using adaptive size gradient descent, 
 * see Frassoldati et al (2008) Journal of industrial and management optimization. 
 */
template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
void
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::EstimateAdapativeBarzilaiBorweinStepSize(double* bestStepSize, double* bestFixedImageStepSize)
{
  double s_s_product = 0.; 
  double s_y_product = 0.; 
  double y_y_product = 0.; 
  
  OutputImageIteratorType currentGradientIterator(this->m_FluidVelocityToDeformationFilter->GetOutput(), this->m_FluidVelocityToDeformationFilter->GetOutput()->GetLargestPossibleRegion());
  AsgdMaskIteratorType* asgdMaskIterator = NULL;
  if (!this->m_AsgdMask.IsNull())
  {
    asgdMaskIterator = new AsgdMaskIteratorType(this->m_AsgdMask, this->m_AsgdMask->GetLargestPossibleRegion());
  }
  if (!m_PreviousGradient.IsNull())
  {
    OutputImageIteratorType previousGradientIterator(this->m_PreviousGradient, this->m_PreviousGradient->GetLargestPossibleRegion());
    if (asgdMaskIterator != NULL)
      asgdMaskIterator->GoToBegin(); 
    for (currentGradientIterator.Begin(), previousGradientIterator.Begin(); 
        !currentGradientIterator.IsAtEnd(); 
        ++currentGradientIterator, ++previousGradientIterator)
    {
      if (asgdMaskIterator != NULL)
      {
        if (asgdMaskIterator->Get() <= 128)
        {
          ++(*asgdMaskIterator); 
          continue; 
        }
        ++(*asgdMaskIterator); 
      }
      OutputImagePixelType y = -(currentGradientIterator.Get()-previousGradientIterator.Get()); 
      OutputImagePixelType s = previousGradientIterator.Get()*this->m_StepSizeHistoryForMovingImage[this->m_CurrentIteration-1]; 
      
      y_y_product += y*y; 
      s_y_product += s*y; 
      s_s_product += s*s; 
    }
  }
  double alpha_bb1 = 0.5*s_s_product/s_y_product; 
  double alpha_bb2 = 0.5*s_y_product/y_y_product; 
  double tau = alpha_bb2/alpha_bb1;     
  if (tau < 0.15)
  {
    *bestStepSize = alpha_bb2; 
  }
  else
  {
    *bestStepSize = alpha_bb1; 
  }
  *bestStepSize = std::min(*bestStepSize, 0.0625/this->m_FluidVelocityToDeformationFilter->GetMaxDeformation()); 
  
  niftkitkInfoMacro(<< "EstimateAdapativeBarzilaiBorweinStepSize():alpha_bb1=" << alpha_bb1 << ",alpha_bb2=" << alpha_bb2 << ",tau=" << tau << "*bestStepSize=" << *bestStepSize); 
  
  if (this->m_IsSymmetric)
  {
    double s_s_product_fixed_image = 0.; 
    double s_y_product_fixed_image = 0.; 
    double y_y_product_fixed_image = 0.; 
    OutputImageIteratorType currentFixedGradientIterator(this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput(), this->m_FluidVelocityToFixedImageDeformationFilter->GetOutput()->GetLargestPossibleRegion());
    if (!m_PreviousFixedImageGradient.IsNull())
    {
      OutputImageIteratorType previousFixedImageGradientIterator(this->m_PreviousFixedImageGradient, this->m_PreviousFixedImageGradient->GetLargestPossibleRegion());
      if (asgdMaskIterator != NULL)
        asgdMaskIterator->GoToBegin(); 
      for (currentFixedGradientIterator.Begin(), previousFixedImageGradientIterator.Begin(); 
          !currentFixedGradientIterator.IsAtEnd(); 
          ++currentFixedGradientIterator, ++previousFixedImageGradientIterator)
      {
        if (asgdMaskIterator != NULL)
        {
          if (asgdMaskIterator->Get() <= 128)
          {
            ++(*asgdMaskIterator); 
            continue; 
          }
          ++(*asgdMaskIterator); 
        }
        OutputImagePixelType y = -(currentFixedGradientIterator.Get()-previousFixedImageGradientIterator.Get()); 
        OutputImagePixelType s = previousFixedImageGradientIterator.Get()*this->m_StepSizeHistoryForFixedImage[this->m_CurrentIteration-1];
        
        y_y_product_fixed_image += y*y; 
        s_y_product_fixed_image += s*y; 
        s_s_product_fixed_image += s*s; 
      }
    }
    double alpha_bb1_fixed_image = 0.5*s_s_product_fixed_image/s_y_product_fixed_image; 
    double alpha_bb2_fixed_image = 0.5*s_y_product_fixed_image/y_y_product_fixed_image; 
    double tau_fixed_image = alpha_bb2_fixed_image/alpha_bb1_fixed_image;     
    if (tau_fixed_image < 0.15)
    {
      *bestFixedImageStepSize = alpha_bb2_fixed_image; 
    }
    else
    {
      *bestFixedImageStepSize = alpha_bb1_fixed_image; 
    }
    *bestFixedImageStepSize = std::min(*bestFixedImageStepSize, 0.0625/this->m_FluidVelocityToFixedImageDeformationFilter->GetMaxDeformation()); 
    niftkitkInfoMacro(<< "EstimateAdapativeBarzilaiBorweinStepSize():alpha_bb1=" << alpha_bb1 << ",alpha_bb2=" << alpha_bb2 << ",tau=" << tau << "*bestFixedImageStepSize=" << *bestFixedImageStepSize); 
  }
}    

/**
 * Compose the new Jacobian using current and regridded parameters. 
 */
template < typename TFixedImage, typename TMovingImage, typename TScalarType, class TDeformationScalar>
void 
FluidGradientDescentOptimizer<TFixedImage,TMovingImage, TScalarType, TDeformationScalar>
::ComposeJacobian()
{                               
  typedef MultiplyImageFilter<JacobianImageType> MultiplyImageFilterType; 
  typename MultiplyImageFilterType::Pointer multiplyFilter = MultiplyImageFilterType::New(); 
  typedef ImageDuplicator<JacobianImageType> JacobianDuplicatorType; 
  typename JacobianDuplicatorType::Pointer duplicator = JacobianDuplicatorType::New(); 
  
  // Compose the jacobian of the forward moving image transform. 
  this->m_DeformableTransform->ComputeMinJacobian(); 
  if (this->m_MovingImageTransformComposedJacobianForward.IsNotNull())
  {
    multiplyFilter->SetInput1(this->m_MovingImageTransformComposedJacobianForward); 
    multiplyFilter->SetInput2(this->m_DeformableTransform->GetJacobianImage()); 
    multiplyFilter->Update(); 
    this->m_MovingImageTransformComposedJacobianForward = multiplyFilter->GetOutput(); 
  }
  else
  {
    duplicator->SetInputImage(this->m_DeformableTransform->GetJacobianImage()); 
    duplicator->Update(); 
    this->m_MovingImageTransformComposedJacobianForward = duplicator->GetOutput(); 
  }
  this->m_MovingImageTransformComposedJacobianForward->DisconnectPipeline(); 
  
  if (m_IsSymmetric)
  {
    // Compose the jacobian of the backward moving image transform. 
    this->m_MovingImageInverseTransform->SetIdentity(); 
    if (this->m_CurrentIteration > 0)
      this->m_DeformableTransform->InvertUsingIterativeFixedPoint(this->m_MovingImageInverseTransform.GetPointer(), 30, 5, 0.005); 
    this->m_MovingImageInverseTransform->ComputeMinJacobian();
    if (this->m_MovingImageTransformComposedJacobianBackward.IsNotNull())
    {
      multiplyFilter->SetInput1(this->m_MovingImageTransformComposedJacobianBackward); 
      multiplyFilter->SetInput2(this->m_MovingImageInverseTransform->GetJacobianImage()); 
      multiplyFilter->Update(); 
      this->m_MovingImageTransformComposedJacobianBackward = multiplyFilter->GetOutput(); 
    }
    else
    {
      duplicator->SetInputImage(this->m_MovingImageInverseTransform->GetJacobianImage()); 
      duplicator->Update(); 
      this->m_MovingImageTransformComposedJacobianBackward = duplicator->GetOutput(); 
    }
    this->m_MovingImageTransformComposedJacobianBackward->DisconnectPipeline(); 
    
    // Compose the jacobian of the forward fixed image transform. 
    this->m_FixedImageTransform->ComputeMinJacobian(); 
    if (this->m_FixedImageTransformComposedJacobianForward.IsNotNull())
    {
      multiplyFilter->SetInput1(this->m_FixedImageTransformComposedJacobianForward); 
      multiplyFilter->SetInput2(this->m_FixedImageTransform->GetJacobianImage()); 
      multiplyFilter->Update(); 
      this->m_FixedImageTransformComposedJacobianForward = multiplyFilter->GetOutput(); 
    }
    else
    {
      duplicator->SetInputImage(this->m_FixedImageTransform->GetJacobianImage()); 
      duplicator->Update(); 
      this->m_FixedImageTransformComposedJacobianForward = duplicator->GetOutput(); 
    }
    this->m_FixedImageTransformComposedJacobianForward->DisconnectPipeline(); 
    
    // Compose the jacobian of the backwarxd fixed image transform. 
    this->m_FixedImageInverseTransform->SetIdentity(); 
    if (this->m_CurrentIteration > 0)
      this->m_FixedImageTransform->InvertUsingIterativeFixedPoint(this->m_FixedImageInverseTransform.GetPointer(), 30, 5, 0.005); 
    this->m_FixedImageInverseTransform->ComputeMinJacobian(); 
    if (this->m_FixedImageTransformComposedJacobianBackward.IsNotNull())
    {
      multiplyFilter->SetInput1(this->m_FixedImageTransformComposedJacobianBackward); 
      multiplyFilter->SetInput2(this->m_FixedImageInverseTransform->GetJacobianImage()); 
      multiplyFilter->Update(); 
      this->m_FixedImageTransformComposedJacobianBackward = multiplyFilter->GetOutput(); 
    }
    else
    {
      duplicator->SetInputImage(this->m_FixedImageInverseTransform->GetJacobianImage()); 
      duplicator->Update(); 
      this->m_FixedImageTransformComposedJacobianBackward = duplicator->GetOutput(); 
    }
    this->m_FixedImageTransformComposedJacobianBackward->DisconnectPipeline(); 
  }
}

    



} // namespace itk.

#endif
