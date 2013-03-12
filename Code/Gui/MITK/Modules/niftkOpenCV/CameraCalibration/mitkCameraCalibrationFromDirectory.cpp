/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkCameraCalibrationFromDirectory.h"
#include "mitkCameraCalibrationFacade.h"
#include <ios>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cv.h>
#include <highgui.h>
#include "FileHelper.h"

namespace mitk {

//-----------------------------------------------------------------------------
CameraCalibrationFromDirectory::CameraCalibrationFromDirectory()
{

}


//-----------------------------------------------------------------------------
CameraCalibrationFromDirectory::~CameraCalibrationFromDirectory()
{

}


//-----------------------------------------------------------------------------
bool CameraCalibrationFromDirectory::Calibrate(const std::string& fullDirectoryName,
    const int& numberCornersX,
    const int& numberCornersY,
    const float& sizeSquareMillimeters,
    const std::string& outputFile,
    const bool& writeImages
    )
{
  bool isSuccessful = false;
  int width = 0;
  int height = 0;

  try
  {
    std::vector<IplImage*> images;
    std::vector<std::string> fileNames;

    std::vector<IplImage*> successfullImages;
    std::vector<std::string> successfullFileNames;

    CvMat *imagePoints = NULL;
    CvMat *objectPoints = NULL;
    CvMat *pointCounts = NULL;

    CvMat *intrinsicMatrix = cvCreateMat(3,3,CV_32FC1);
    CvMat *distortionCoeffs = cvCreateMat(5, 1, CV_32FC1);

    LoadChessBoardsFromDirectory(fullDirectoryName, images, fileNames);

    CheckConstImageSize(images, width, height);
    CvSize imageSize = cvGetSize(images[0]);

    ExtractChessBoardPoints(images, fileNames, numberCornersX, numberCornersY, writeImages, successfullImages, successfullFileNames, imagePoints, objectPoints, pointCounts);

    int numberOfSuccessfulViews = successfullImages.size();
    CvMat *rotationVectors = cvCreateMat(numberOfSuccessfulViews, 3,CV_32FC1);
    CvMat *translationVectors = cvCreateMat(numberOfSuccessfulViews, 3, CV_32FC1);

    double projectionError = CalibrateSingleCameraParameters(
        numberOfSuccessfulViews,
        *objectPoints,
        *imagePoints,
        *pointCounts,
        imageSize,
        *intrinsicMatrix,
        *distortionCoeffs,
        *rotationVectors,
        *translationVectors
        );

    std::ostream *os = NULL;
    std::ostringstream oss;
    std::ofstream fs;

    if (outputFile.size() > 0)
    {
      fs.open(outputFile.c_str(), std::ios::out);
      if (!fs.fail())
      {
        os = &fs;
        std::cout << "Writing to " << outputFile << std::endl;
      }
      else
      {
        std::cerr << "ERROR: Writing calibration data to file " << outputFile << " failed!" << std::endl;
      }
    }
    else
    {
      os = &oss;
    }

    *os << "Mono calibration" << std::endl;
    OutputCalibrationData(
        *os,
        *objectPoints,
        *imagePoints,
        *pointCounts,
        *intrinsicMatrix,
        *distortionCoeffs,
        *rotationVectors,
        *translationVectors,
        projectionError,
        width,
        height,
        numberCornersX,
        numberCornersY,
        successfullFileNames
        );

    // Also output these as XML, as they are used in niftkCorrectVideoDistortion
    cvSave(std::string(outputFile + ".intrinsic.xml").c_str(), intrinsicMatrix);
    cvSave(std::string(outputFile + ".distortion.xml").c_str(), distortionCoeffs);

    // Tidy up.
    if(fs.is_open())
    {
      fs.close();
    }
    cvReleaseMat(&imagePoints);
    cvReleaseMat(&objectPoints);
    cvReleaseMat(&pointCounts);
    cvReleaseMat(&intrinsicMatrix);
    cvReleaseMat(&distortionCoeffs);
    cvReleaseMat(&rotationVectors);
    cvReleaseMat(&translationVectors);

    for (unsigned int i = 0; i < images.size(); i++)
    {
      cvReleaseImage(&images[i]);
    }

    isSuccessful = true;
  }
  catch(std::logic_error& e)
  {
    std::cerr << "CameraCalibrationFromDirectory::Calibrate: exception thrown e=" << e.what() << std::endl;
  }

  return isSuccessful;
}

} // end namespace
