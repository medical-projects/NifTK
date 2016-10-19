/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkDistanceFromCamera.h"
#include <mitkCameraCalibrationFacade.h>
#include <niftkOpenCVImageConversion.h>
#include <niftkMathsUtils.h>
#include <cv.h>
#include <opencv2/nonfree/features2d.hpp>

namespace niftk
{

class DistanceFromCameraPrivate {

public:

  DistanceFromCameraPrivate() {}
  ~DistanceFromCameraPrivate() {}

  double GetDistance(const mitk::DataNode::Pointer& leftImage,
                     const mitk::DataNode::Pointer& rightImage
                    );

  double GetDistance(const mitk::Image::Pointer& leftImage,
                     const mitk::Image::Pointer& rightImage,
                     const mitk::CameraIntrinsics::Pointer& leftIntrinsic,
                     const mitk::CameraIntrinsics::Pointer& rightIntrinsic,
                     const itk::Matrix<float, 4, 4>& stereoExtrinsics
                    );

private:

  cv::Mat m_LeftGreyScale;
  cv::Mat m_RightGreyScale;

};

//-----------------------------------------------------------------------------
double DistanceFromCameraPrivate::GetDistance(const mitk::DataNode::Pointer& leftImage,
                                              const mitk::DataNode::Pointer& rightImage
                                             )
{
  return 0;
}


//-----------------------------------------------------------------------------
double DistanceFromCameraPrivate::GetDistance(const mitk::Image::Pointer& leftImage,
                                              const mitk::Image::Pointer& rightImage,
                                              const mitk::CameraIntrinsics::Pointer& leftIntrinsic,
                                              const mitk::CameraIntrinsics::Pointer& rightIntrinsic,
                                              const itk::Matrix<float, 4, 4>& stereoExtrinsics
                                             )
{
  cv::Mat leftWrapper = niftk::MitkImageToOpenCVMat(leftImage);
  cv::Mat rightWrapper = niftk::MitkImageToOpenCVMat(rightImage);

  cv::cvtColor(leftWrapper, m_LeftGreyScale, CV_RGB2GRAY);
  cv::cvtColor(rightWrapper, m_RightGreyScale, CV_RGB2GRAY);

  cv::SIFT detector;

  std::vector<cv::KeyPoint> leftKeyPoints;
  detector.detect(m_LeftGreyScale, leftKeyPoints);

  std::vector<cv::KeyPoint> rightKeyPoints;
  detector.detect(m_RightGreyScale, rightKeyPoints);

  cv::SiftDescriptorExtractor extractor;

  cv::Mat leftDescriptors;
  cv::Mat rightDescriptors;

  extractor.compute( m_LeftGreyScale, leftKeyPoints, leftDescriptors );
  extractor.compute( m_RightGreyScale, rightKeyPoints, rightDescriptors );

  cv::BFMatcher matcher(cv::NORM_L2);
  std::vector< cv::DMatch > matches;
  matcher.match(leftDescriptors, rightDescriptors, matches);

  std::map<float, std::pair<int, int> > mapLeftToRight;
  for (unsigned int i = 0; i < matches.size(); i++)
  {
    mapLeftToRight.insert(std::pair<float, std::pair<int, int> >
                         (matches[i].distance,
                          std::pair<int, int>(matches[i].queryIdx, matches[i].trainIdx)));
  }

  // Extract top 20 best matching points.
  int numberOfPoints = 20;
  std::vector< std::pair<cv::Point2d, cv::Point2d> > pointPairs;
  std::vector< std::pair < cv::Point3d, double > > pointsIn3D;

  std::map<float, std::pair<int, int> >::iterator iter;
  for (iter = mapLeftToRight.begin(); iter != mapLeftToRight.end() && pointPairs.size() < numberOfPoints; ++iter)
  {
    cv::Point2d leftPoint = leftKeyPoints[iter->second.first].pt;
    cv::Point2d rightPoint = rightKeyPoints[iter->second.second].pt;

    pointPairs.push_back(std::pair<cv::Point2d, cv::Point2d>(leftPoint,rightPoint));
  }

  // Triangulate
  cv::Mat rightToLeftRotationMatrix = cvCreateMat (3,3,CV_64FC1);
  cv::Mat rightToLeftTranslationVector = cvCreateMat (1,3,CV_64FC1);
  cv::Mat leftInt = leftIntrinsic->GetCameraMatrix();
  cv::Mat rightInt = rightIntrinsic->GetCameraMatrix();

  for (int r = 0; r < 3; r++)
  {
    for (int c = 0; c < 3; c++)
    {
      rightToLeftRotationMatrix.at<double>(r, c) = stereoExtrinsics[r][c];
    }
    rightToLeftTranslationVector.at<double>(0, r) = stereoExtrinsics[r][3];
  }

  pointsIn3D = mitk::TriangulatePointPairsUsingGeometry(
    pointPairs,
    leftInt,
    rightInt,
    rightToLeftRotationMatrix,
    rightToLeftTranslationVector,
    std::numeric_limits<int>::max());

  // Take median z-distance of non negative numbers.
  std::vector<double> zdist;
  for (int i = 0; i < pointsIn3D.size(); i++)
  {
    if (pointsIn3D[i].first.z > 0)
    {
      zdist.push_back(pointsIn3D[i].first.z);
    }
  }

  return niftk::Median(zdist);
}


//-----------------------------------------------------------------------------
DistanceFromCamera::DistanceFromCamera()
: m_Impl(new DistanceFromCameraPrivate())
{
}


//-----------------------------------------------------------------------------
DistanceFromCamera::~DistanceFromCamera()
{
}


//-----------------------------------------------------------------------------
double DistanceFromCamera::GetDistance(const mitk::DataNode::Pointer& leftImage,
                                       const mitk::DataNode::Pointer& rightImage
                                      )
{
  return m_Impl->GetDistance(leftImage, rightImage);
}


//-----------------------------------------------------------------------------
double DistanceFromCamera::GetDistance(const mitk::Image::Pointer& leftImage,
                                       const mitk::Image::Pointer& rightImage,
                                       const mitk::CameraIntrinsics::Pointer& leftIntrinsic,
                                       const mitk::CameraIntrinsics::Pointer& rightIntrinsic,
                                       const itk::Matrix<float, 4, 4>& stereoExtrinsics
                                      )
{
  return m_Impl->GetDistance(leftImage, rightImage, leftIntrinsic, rightIntrinsic, stereoExtrinsics);
}

} // end namespace
