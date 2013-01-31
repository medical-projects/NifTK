/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date$
 Revision          : $Revision$
 Last modified by  : $Author$

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include <cstdlib>
#include "mitkCameraCalibrationFromDirectory.h"
#include "niftkCameraCalibrationCLP.h"

int main(int argc, char** argv)
{
  PARSE_ARGS;

  if ( leftCameraInputDirectory.length() == 0 || outputCalibrationData.length() == 0 )
  {
    commandLine.getOutput()->usage(commandLine);
    return EXIT_FAILURE;
  }

  mitk::CameraCalibrationFromDirectory::Pointer calibrationObject = mitk::CameraCalibrationFromDirectory::New();
  calibrationObject->Calibrate(leftCameraInputDirectory, xCorners, yCorners, size, outputCalibrationData);

  return EXIT_SUCCESS;
}
