/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef MITKSTEREOCAMERACALIBRATIONFROMTWODIRECTORIES_H
#define MITKSTEREOCAMERACALIBRATIONFROMTWODIRECTORIES_H

#include "niftkOpenCVExports.h"
#include <string>
#include <itkObject.h>
#include <itkObjectFactory.h>
#include <mitkCommon.h>

namespace mitk {

/**
 * \class StereoCameraCalibrationFromTwoDirectories
 * \brief Does a stereo camera calibration from two directories, each containing a set of image files.
 */
class NIFTKOPENCV_EXPORT StereoCameraCalibrationFromTwoDirectories : public itk::Object
{

public:

  mitkClassMacro(StereoCameraCalibrationFromTwoDirectories, itk::Object);
  itkNewMacro(StereoCameraCalibrationFromTwoDirectories);

  bool Calibrate(const std::string& leftDirectoryName,
      const std::string& rightDirectoryName,
      const int& numberCornersX,
      const int& numberCornersY,
      const float& sizeSquareMillimeters,
      const std::string& outputFileName,
      const bool& writeImages
      );

protected:

  StereoCameraCalibrationFromTwoDirectories();
  virtual ~StereoCameraCalibrationFromTwoDirectories();

  StereoCameraCalibrationFromTwoDirectories(const StereoCameraCalibrationFromTwoDirectories&); // Purposefully not implemented.
  StereoCameraCalibrationFromTwoDirectories& operator=(const StereoCameraCalibrationFromTwoDirectories&); // Purposefully not implemented.

}; // end class

} // end namespace

#endif
