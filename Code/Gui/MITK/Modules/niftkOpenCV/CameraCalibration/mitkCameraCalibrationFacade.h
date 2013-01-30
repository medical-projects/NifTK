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

#ifndef MITKCAMERACALIBRATIONFACADE_H
#define MITKCAMERACALIBRATIONFACADE_H

#include <cv.h>
#include <cstdlib>

/**
 * \file mitkCameraCalibrationFacade
 * \brief Interface to OpenCV camera calibration.
 */
namespace mitk {

/**
 * \brief Uses OpenCV to load chessboard images from a directory.
 * \throw Throws logic_error if fullDirectoryName is not a valid directory,
 * the directory contains no files, or the files are not images that OpenCV recognises.
 * \param images output parameter containing the images, which the caller must de-allocate.
 * \param fileNames output parameter containing the corresponding filenames.
 */
 void LoadChessBoardsFromDirectory(const std::string& fullDirectoryName,
                                   std::vector<IplImage*>& images,
                                   std::vector<std::string>& fileNames);


/**
 * \brief Iterates through the list of images, checking that the width and height are consistent.
 * \throw Throws logic_error if images are of differing sizes.
 * \param width output parameter containing the image width for all the images
 * \param height output parameter containing the image height for all the images
 */
void CheckConstImageSize(const std::vector<IplImage*>& images, int& width, int& height);


/**
 * \brief Extracts the chess board points, using OpenCV routines.
 * \param images vector of images, of all the same size
 * \param fileNames the corresponding file names, must be the same length vector as images.
 * \param numberCornersWidth the number of internal corners along the width axis (X).
 * \param numberCornersHeight the number of internal corners along the height axis (Y).
 * \param drawCorners if true will dump images to indicate which points were found.
 * \param outputImages list of successfully processed images, which are just pointers back to the same images as in the first parameter.
 * \param outputFileNames corresponding list of successfully processed images filenames,
 * \param outputImagePoints output image points, array size = number of successes*numberOfCorners x 3, and caller must de-allocate.
 * \param outputObjectPoints output object points, array size = number of successes*numberOfCorners x 2, and caller must de-allocate.
 * \param outputPointCounts output point counts, array size = number of successes x 1, and caller must de-allocate.
 */
void ExtractChessBoardPoints(const std::vector<IplImage*>& images,
                             const std::vector<std::string>& fileNames,
                             const int& numberCornersWidth,
                             const int& numberCornersHeight,
                             const bool& drawCorners,
                             std::vector<IplImage*>& outputImages,
                             std::vector<std::string>& outputFileNames,
                             CvMat*& outputImagePoints,
                             CvMat*& outputObjectPoints,
                             CvMat*& outputPointCounts
                             );


/**
 * \brief Calibrate a single cameras intrinsic parameters.
 * \param objectPoints list of 3D points generated as output from ExtractChessBoardPoints.
 * \param imagePoints list of 2D image points generated as output from ExtractChessBoardPoints.
 * \param pointCounts array containing the number of points that matched, which should all be equal.
 * \param outputIntrinsicMatrix 3 x 3 matrix.
 * \param outputDistortionCoefficients 5 x 1 matrix of (k1, k2, p1, p2, k3).
 */
double CalibrateSingleCameraIntrinsicParameters(
       const CvMat& objectPoints,
       const CvMat& imagePoints,
       const CvMat& pointCounts,
       const CvSize& imageSize,
       CvMat& outputIntrinsicMatrix,
       CvMat& outputDistortionCoefficients
       );


/**
 * \brief Calibrate a single cameras extrinsic parameters.
 * \param objectPoints list of 3D points generated as output from ExtractChessBoardPoints.
 * \param imagePoints list of 2D image points generated as output from ExtractChessBoardPoints.
 * \param intrinsicMatrix 3 x 3 matrix.
 * \param distortionCoefficients 5 x 1 matrix of (k1, k2, p1, p2, k3).
 * \param outputRotationMatrix the rotation matrix - see OpenCV docs - its a 3x3 not a 1x3.
 * \param outputTranslationVector the 1 x 3 translation vector.
 */
void CalibrateSingleCameraExtrinsicParameters(
     const CvMat& objectPoints,
     const CvMat& imagePoints,
     const CvMat& intrinsicMatrix,
     const CvMat& distortionCoefficients,
     CvMat& outputRotationMatrix,
     CvMat& outputTranslationVector
     );


/**
 * \brief Calibrate a single camera for both intrinsic and extrinsic parameters.
 * \param numberSuccessfulViews the number of successful views from ExtractChessBoardPoints.
 * \param objectPoints list of 3D points generated as output from ExtractChessBoardPoints.
 * \param imagePoints list of 2D image points generated as output from ExtractChessBoardPoints.
 * \param pointCounts array containing the number of points that matched, which should all be equal.
 * \param outputIntrinsicMatrix 3 x 3 matrix.
 * \param outputDistortionCoefficients 5 x 1 matrix of (k1, k2, p1, p2, k3).
 * \param outputRotationMatrix the rotation matrix - see OpenCV docs - its a 3x3 not a 1x3.
 * \param outputTranslationVector the 1 x 3 translation vector.
 */
double CalibrateSingleCameraParameters(
     const int& numberSuccessfulViews,
     const CvMat& objectPoints,
     const CvMat& imagePoints,
     const CvMat& pointCounts,
     const CvSize& imageSize,
     CvMat& outputIntrinsicMatrix,
     CvMat& outputDistortionCoefficients,
     CvMat& outputRotationMatrix,
     CvMat& outputTranslationVector
     );

} // end namespace




#endif // MITKCAMERACALIBRATIONFACADE_H
