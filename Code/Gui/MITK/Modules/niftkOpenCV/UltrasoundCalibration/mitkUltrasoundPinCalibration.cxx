/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkUltrasoundPinCalibration.h"
#include <mitkFileIOUtils.h>
#include <niftkFileHelper.h>
#include <itkUltrasoundPinCalibrationCostFunction.h>
#include <itkLevenbergMarquardtOptimizer.h>
#include <mitkCameraCalibrationFacade.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <mitkOpenCVMaths.h>
#include <algorithm>

namespace mitk {

//-----------------------------------------------------------------------------
UltrasoundPinCalibration::UltrasoundPinCalibration()
{
}


//-----------------------------------------------------------------------------
UltrasoundPinCalibration::~UltrasoundPinCalibration()
{
}


//-----------------------------------------------------------------------------
bool UltrasoundPinCalibration::CalibrateUsingInvariantPointAndFilesInTwoDirectories(
    const std::string& matrixDirectory,
    const std::string& pointDirectory,
    const bool& optimiseScaling,
    const bool& optimiseInvariantPoint,
    const vtkMatrix4x4& rigidBodyInitialGuess,
    std::vector<double>& rigidBodyTransformation,
    mitk::Point3D& invariantPoint,
    double& millimetresPerPixel,
    double& residualError,
    vtkMatrix4x4& outputMatrix
    )
{
  std::vector<std::string> matrixFiles = niftk::GetFilesInDirectory(matrixDirectory);
  std::sort(matrixFiles.begin(), matrixFiles.end());

  std::vector<std::string> pointFiles = niftk::GetFilesInDirectory(pointDirectory);
  std::sort(pointFiles.begin(), pointFiles.end());

  if (matrixFiles.size() != pointFiles.size())
  {
    MITK_ERROR << "ERROR: The matrix directory:" << std::endl << "  " << matrixDirectory << std::endl << "and the point directory:" << std::endl << "  " << pointDirectory << "contain a different number of files!" << std::endl;
    return false;
  }

  std::vector<cv::Mat> matrices = LoadMatricesFromDirectory (matrixDirectory);

  std::vector<cv::Point3d> points;
  for (unsigned int i = 0; i < pointFiles.size(); i++)
  {
    mitk::Point2D point;
    if (mitk::Load2DPointFromFile(pointFiles[i], point))
    {
      cv::Point3d cvPoint;
      cvPoint.x = point[0];
      cvPoint.y = point[1];
      cvPoint.z = 0.0;
      points.push_back(cvPoint);
    }
  }

  if (matrices.size() != matrixFiles.size())
  {
    MITK_ERROR << "ERROR: Failed to load all the matrices in directory:" << matrixDirectory << std::endl;
    return false;
  }

  if (points.size() != pointFiles.size())
  {
    MITK_ERROR << "ERROR: Failed to load all the points in directory:" << pointDirectory << std::endl;
    return false;
  }

  cv::Matx44d initialGuess;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      initialGuess(i,j) = rigidBodyInitialGuess.GetElement(i, j);
    }
  }

  cv::Matx44d transformationMatrix;
  cv::Point3d invPoint(invariantPoint[0], invariantPoint[1], invariantPoint[2]);

  bool calibratedSuccessfully = this->Calibrate(
      matrices,
      points,
      optimiseScaling,
      optimiseInvariantPoint,
      initialGuess,
      rigidBodyTransformation,
      invPoint,
      millimetresPerPixel,
      transformationMatrix,
      residualError
      );

  if (!calibratedSuccessfully)
  {
    MITK_ERROR << "CalibrateUsingTrackerPointAndFilesInTwoDirectories: Failed to calibrate successfully" << std::endl;
    return false;
  }

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      outputMatrix.SetElement(i, j, transformationMatrix(i, j));
    }
  }
  invariantPoint[0] = invPoint.x;
  invariantPoint[1] = invPoint.y;
  invariantPoint[2] = invPoint.z;
  return true;
}


//-----------------------------------------------------------------------------
bool UltrasoundPinCalibration::Calibrate(
    const std::vector< cv::Mat >& matrices,
    const std::vector< cv::Point3d >& points,
    const bool& optimiseScaling,
    const bool& optimiseInvariantPoint,
    const cv::Matx44d& rigidBodyInitialGuess,
    std::vector<double>& rigidBodyTransformation,
    cv::Point3d& invariantPoint,
    double& millimetresPerPixel,
    cv::Matx44d& outputMatrix,
    double& residualError
    )
{
  bool isSuccessful = false;

  itk::UltrasoundPinCalibrationCostFunction::ParametersType parameters;
  itk::UltrasoundPinCalibrationCostFunction::ParametersType scaleFactors;
  
  if (!optimiseScaling && !optimiseInvariantPoint)
  {
    parameters.SetSize(6);
    scaleFactors.SetSize(6);
  }
  else if (!optimiseScaling && optimiseInvariantPoint)
  {
    parameters.SetSize(9);
    parameters[6] = invariantPoint.x;
    parameters[7] = invariantPoint.y;
    parameters[8] = invariantPoint.z;
    
    scaleFactors.SetSize(9);
    scaleFactors[6] = 1;
    scaleFactors[7] = 1;
    scaleFactors[8] = 1;
  }
  else if (optimiseScaling && !optimiseInvariantPoint)
  {
    parameters.SetSize(7);
    parameters[6] = millimetresPerPixel;
    
    scaleFactors.SetSize(7);
    scaleFactors[6] = 0.0001;
  }
  else if (optimiseScaling && optimiseInvariantPoint)
  {
    parameters.SetSize(10);
    parameters[6] = millimetresPerPixel;
    parameters[7] = invariantPoint.x;
    parameters[8] = invariantPoint.y;
    parameters[9] = invariantPoint.z;
    
    scaleFactors.SetSize(10);
    scaleFactors[6] = 0.0001;
    scaleFactors[7] = 1;
    scaleFactors[8] = 1;
    scaleFactors[9] = 1;
  }

  parameters[0] = rigidBodyTransformation[0];
  parameters[1] = rigidBodyTransformation[1];
  parameters[2] = rigidBodyTransformation[2];
  parameters[3] = rigidBodyTransformation[3];
  parameters[4] = rigidBodyTransformation[4];
  parameters[5] = rigidBodyTransformation[5];

  scaleFactors[0] = 0.1;
  scaleFactors[1] = 0.1;
  scaleFactors[2] = 0.1;
  scaleFactors[3] = 1;
  scaleFactors[4] = 1;
  scaleFactors[5] = 1;
  
  std::cout << "UltrasoundPinCalibration:Start parameters = " << parameters << std::endl;
  std::cout << "UltrasoundPinCalibration:Start scale factors = " << scaleFactors << std::endl;
  
  itk::UltrasoundPinCalibrationCostFunction::Pointer costFunction = itk::UltrasoundPinCalibrationCostFunction::New();
  costFunction->SetMatrices(matrices);
  costFunction->SetPoints(points);
  costFunction->SetNumberOfParameters(parameters.GetSize());
  costFunction->SetInvariantPoint(invariantPoint);
  costFunction->SetMillimetresPerPixel(millimetresPerPixel);
  costFunction->SetScales(scaleFactors);
  costFunction->SetInitialGuess(rigidBodyInitialGuess);

  itk::LevenbergMarquardtOptimizer::Pointer optimizer = itk::LevenbergMarquardtOptimizer::New();
  optimizer->SetCostFunction(costFunction);
  optimizer->SetInitialPosition(parameters);
  optimizer->SetScales(scaleFactors);
  optimizer->SetNumberOfIterations(20000000);
  optimizer->UseCostFunctionGradientOn();
  optimizer->SetGradientTolerance(0.0000005);
  optimizer->SetEpsilonFunction(0.0000005);
  optimizer->SetValueTolerance(0.0000005);

  optimizer->StartOptimization();

  parameters = optimizer->GetCurrentPosition();
  outputMatrix = costFunction->GetCalibrationTransformation(parameters);

  itk::UltrasoundPinCalibrationCostFunction::MeasureType values = costFunction->GetValue(parameters);
  residualError = costFunction->GetResidual(values);

  std::cerr << "Stop condition:" << optimizer->GetStopConditionDescription();

  if (optimiseScaling)
  {
    millimetresPerPixel = parameters[6];
  }
  if (!optimiseScaling && optimiseInvariantPoint)
  {
    invariantPoint.x = parameters[6];
    invariantPoint.y = parameters[7];
    invariantPoint.z = parameters[8];
  }
  if (optimiseScaling && optimiseInvariantPoint)
  {
    invariantPoint.x = parameters[7];
    invariantPoint.y = parameters[8];
    invariantPoint.z = parameters[9];
  }
  
  isSuccessful = true;

  std::cout << "UltrasoundPinCalibration:Final parameters = " << parameters << std::endl;
  std::cout << "UltrasoundPinCalibration:Final mm/pix = " << millimetresPerPixel << std::endl;
  std::cout << "UltrasoundPinCalibration:Final residual = " << residualError << std::endl;
  std::cout << "UltrasoundPinCalibration:Result:" << std::endl;
  for (int i = 0; i < 4; i++)
  {
    std::cout << outputMatrix(i, 0) << " " << outputMatrix(i, 1) << " " << outputMatrix(i, 2) << " " << outputMatrix(i, 3) << std::endl;
  }
    
  return isSuccessful;
}

//-----------------------------------------------------------------------------
} // end namespace
