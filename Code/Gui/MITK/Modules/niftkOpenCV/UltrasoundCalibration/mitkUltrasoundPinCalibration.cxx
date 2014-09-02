/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkUltrasoundPinCalibration.h"
#include <itkUltrasoundPinCalibrationCostFunction.h>
#include <itkLevenbergMarquardtOptimizer.h>
#include <cassert>

namespace mitk {

//-----------------------------------------------------------------------------
UltrasoundPinCalibration::UltrasoundPinCalibration()
{
  m_CostFunction = itk::UltrasoundPinCalibrationCostFunction::New();
  m_DownCastCostFunction = dynamic_cast<itk::UltrasoundPinCalibrationCostFunction*>(m_CostFunction.GetPointer());
  assert(m_DownCastCostFunction);
  this->Modified();
}


//-----------------------------------------------------------------------------
UltrasoundPinCalibration::~UltrasoundPinCalibration()
{
}


//-----------------------------------------------------------------------------
void UltrasoundPinCalibration::SetImageScaleFactors(const mitk::Point2D& point)
{
  m_DownCastCostFunction->SetScaleFactors(point);
  this->Modified();
}


//-----------------------------------------------------------------------------
mitk::Point2D UltrasoundPinCalibration::GetImageScaleFactors() const
{
  return m_DownCastCostFunction->GetScaleFactors();
}


//-----------------------------------------------------------------------------
void UltrasoundPinCalibration::SetOptimiseImageScaleFactors(const bool& optimise)
{
  m_DownCastCostFunction->SetOptimiseScaleFactors(optimise);
  this->Modified();
}


//-----------------------------------------------------------------------------
bool UltrasoundPinCalibration::GetOptimiseImageScaleFactors() const
{
  return m_DownCastCostFunction->GetOptimiseScaleFactors();
}


//-----------------------------------------------------------------------------
double UltrasoundPinCalibration::Calibrate()
{
  double residualError = 0;

  itk::UltrasoundPinCalibrationCostFunction::ParametersType parameters;
  itk::UltrasoundPinCalibrationCostFunction::ParametersType scaleFactors;
  
  // Setup size of parameters array.
  int numberOfParameters = 6;
  if (this->GetOptimiseImageScaleFactors())
  {
    numberOfParameters += 2;
  }
  if (this->GetOptimiseInvariantPoint())
  {
    numberOfParameters += 3;
  }
  if (this->GetOptimiseTimingLag())
  {
    numberOfParameters += 1;
  }
  assert(numberOfParameters == 6
         || numberOfParameters == 9
         || numberOfParameters == 11
         || numberOfParameters == 12
         );

  parameters.SetSize(numberOfParameters);
  scaleFactors.SetSize(numberOfParameters);

  parameters.Fill(0);

  parameters[0] = m_RigidTransformation[0];
  parameters[1] = m_RigidTransformation[1];
  parameters[2] = m_RigidTransformation[2];
  parameters[3] = m_RigidTransformation[3];
  parameters[4] = m_RigidTransformation[4];
  parameters[5] = m_RigidTransformation[5];

  if (this->GetOptimiseInvariantPoint())
  {
    mitk::Point3D invariantPoint = this->GetInvariantPoint();
    parameters[6] = invariantPoint[0];
    parameters[7] = invariantPoint[1];
    parameters[8] = invariantPoint[2];
  }
  if (this->GetOptimiseImageScaleFactors())
  {
    mitk::Point2D scaleFactors = this->GetImageScaleFactors();
    parameters[9] = scaleFactors[0];
    parameters[10] = scaleFactors[1];
  }
  if (this->GetOptimiseTimingLag())
  {
    TimeStampType timeStamp = this->GetTimingLag();
    parameters[11] = timeStamp;
  }
  
  std::cout << "UltrasoundPinCalibration:Start parameters = " << parameters << std::endl;
  
  m_CostFunction->SetNumberOfParameters(parameters.GetSize());
  m_CostFunction->SetScales(scaleFactors);

  itk::LevenbergMarquardtOptimizer::Pointer optimizer = itk::LevenbergMarquardtOptimizer::New();
  optimizer->UseCostFunctionGradientOn();
  optimizer->SetCostFunction(m_CostFunction);
  optimizer->SetInitialPosition(parameters);
  optimizer->SetScales(scaleFactors);
  optimizer->SetNumberOfIterations(20000000);
  optimizer->SetGradientTolerance(0.0000005);
  optimizer->SetEpsilonFunction(0.0000005);
  optimizer->SetValueTolerance(0.0000005);

  optimizer->StartOptimization();

  parameters = optimizer->GetCurrentPosition();

  m_RigidTransformation[0] = parameters[0];
  m_RigidTransformation[1] = parameters[1];
  m_RigidTransformation[2] = parameters[2];
  m_RigidTransformation[3] = parameters[3];
  m_RigidTransformation[4] = parameters[4];
  m_RigidTransformation[5] = parameters[5];

  if (this->GetOptimiseInvariantPoint())
  {
    mitk::Point3D invariantPoint;
    invariantPoint[0] = parameters[6];
    invariantPoint[1] = parameters[7];
    invariantPoint[2] = parameters[8];
    this->SetInvariantPoint(invariantPoint);
  }
  if (this->GetOptimiseImageScaleFactors())
  {
    mitk::Point2D scaleFactors;
    scaleFactors[0] = parameters[9];
    scaleFactors[1] = parameters[10];
    this->SetImageScaleFactors(scaleFactors);
  }
  if (this->GetOptimiseTimingLag())
  {
    TimeStampType timeStamp;
    timeStamp = parameters[11];
    this->SetTimingLag(timeStamp);
  }

  itk::UltrasoundPinCalibrationCostFunction::MeasureType values = m_CostFunction->GetValue(parameters);
  residualError = m_CostFunction->GetResidual(values);

  std::cout << "Stop condition:" << optimizer->GetStopConditionDescription();
  std::cout << "UltrasoundPinCalibration:End parameters = " << parameters << std::endl;
  std::cout << "UltrasoundPinCalibration:End residual = " << residualError << std::endl;

  return residualError;
}

//-----------------------------------------------------------------------------
} // end namespace
