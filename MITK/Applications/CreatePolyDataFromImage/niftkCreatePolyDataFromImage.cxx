/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

/*!
 * \file niftkCreatePolyDataFromImage.cxx
 * \page niftkCreatePolyDataFromImage
 * \section niftkCreatePolyDataFromImageSummary niftkCreatePolyDataFromImage creates poly data by thresholding the input target image and applying marching cubes.
 */

#include <itkCommandLineHelper.h>
#include <niftkCommandLineParser.h>

#include <mitkIOUtil.h>

#include <niftkImageToSurfaceFilter.h>
#include <niftkLogHelper.h>


struct niftk::CommandLineArgumentDescription clArgList[] = 
{
  {OPT_STRING|OPT_REQ, "i", "fileName", "Input image."},
  {OPT_STRING|OPT_REQ, "o", "fileName", "Output .stl model."},
  {OPT_DOUBLE, "t", "double", "[100.0] Threshold value."},
  {OPT_INT, "e", "int", "[0] Extraction type either (0) vtk Marching cubes or (1) corrected marching cubes 33."},
  {OPT_DOUBLE, "ir", "double", "[1.0] Input image downsampling ratio."},
  {OPT_INT, "isType", "int", "[0] Input image smoothing either (0) no smoothing, (1) Gaussian smoothing, or"
    "(2) Median smoothing."},
  {OPT_FLOAT, "isRad", "float", "[0.5] Input image smoothing radius."},
  {OPT_INT, "isIter", "int", "[1] Input image smoothing number of iterations."},
  {OPT_INT, "d", "int", "[0] surface decimation type either (0) no decimation, (1) Decimate Pro, (2) Quadratic VTK, "
    "(3) Quadratic, (4) Quadratic Tri, (5) Melax, or (6) Shortest Edge."},
  {OPT_INT, "ssType", "int", "[0] output surface smoothing either (0) no smoothing, (1) Taubin smoothing,"
    "(2) curvature normal smoothing, (3) inverse edge length smoothing, (4) Windowed Sinc smoothing, (5) standard VTK smoothing."},
  {OPT_FLOAT, "ssRad", "float", "[0.5] Output surface smoothing radius."},
  {OPT_INT, "ssIter", "int", "[1] Output surface smoothing number of iterations."},
  {OPT_FLOAT, "tarRed", "float", "[0.1] Polydata reduction ratio."},
  {OPT_SWITCH, "cleanSurfOn", NULL, "Turn on small object removal."},
  {OPT_INT, "cleanT", "int", "[1000] Polygon threshold for small object removal."},
  {OPT_DONE, NULL, NULL, 
    "Program to extract a polydata object from an image using corrected marching cubes 33.\n"
  }
};


enum {
  O_INPUT_IMAGE,

  O_OUTPUT_POLYDATA, 

  O_THRESHOLD, 

  O_EXTRACTION_TYPE, 

  O_SAMPLE_RATIO,

  O_INPUT_SMOOTH_TYPE,
  
  O_INPUT_SMOOTH_RAD,

  O_INPUT_SMOOTH_ITER,

  O_DECIMATION, 

  O_SURF_SMOOTH_TYPE,

  O_SURF_SMOOTH_RAD,

  O_SURF_SMOOTH_ITER, 

  O_TARGET_RED, 

  O_USE_CLEAN_SURF,

  O_CLEAN_THRESHOLD 
};

//-----------------------------------------------------------------------
// main()
// -------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  std::string fileInputImage;
  std::string fileOutputPolyData;
  double threshold = 100.0;
  int    extractionType = 0;
  double  samplingRatio = 1.0;
  int    inputSmoothingType = 0;
  int    inputSmoothingIterations = 1;
  float  inputSmoothingRadius = 0.5f;
  int    decimationType = 0;
  float  targetReduction = 0.1f;
  int    surfSmoothingType = 0;
  int    surfSmoothingIterations = 1;
  float  surfSmoothingRadius = 0.5f;
  bool   useSurfClean = false;
  int    surfCleanThreshold = 1000;

  niftk::CommandLineParser CommandLineOptions(argc, argv, clArgList, false);

  CommandLineOptions.GetArgument(O_INPUT_IMAGE, fileInputImage);

  CommandLineOptions.GetArgument(O_OUTPUT_POLYDATA, fileOutputPolyData);

  CommandLineOptions.GetArgument(O_THRESHOLD, threshold);

  CommandLineOptions.GetArgument(O_EXTRACTION_TYPE, extractionType);

  CommandLineOptions.GetArgument(O_SAMPLE_RATIO, samplingRatio);

  CommandLineOptions.GetArgument(O_INPUT_SMOOTH_TYPE, inputSmoothingType);
  
  CommandLineOptions.GetArgument(O_INPUT_SMOOTH_RAD, inputSmoothingRadius);

  CommandLineOptions.GetArgument(O_INPUT_SMOOTH_ITER, inputSmoothingIterations);

  CommandLineOptions.GetArgument(O_DECIMATION, decimationType);

  CommandLineOptions.GetArgument(O_SURF_SMOOTH_TYPE, surfSmoothingType);

  CommandLineOptions.GetArgument(O_SURF_SMOOTH_RAD, surfSmoothingRadius);

  CommandLineOptions.GetArgument(O_SURF_SMOOTH_ITER, surfSmoothingIterations);

  CommandLineOptions.GetArgument(O_TARGET_RED, targetReduction);

  CommandLineOptions.GetArgument(O_USE_CLEAN_SURF, useSurfClean);

  CommandLineOptions.GetArgument(O_CLEAN_THRESHOLD, surfCleanThreshold);

    // load in image
  mitk::Image::Pointer inputImage = mitk::IOUtil::LoadImage(fileInputImage);
  if (inputImage.IsNull())
  {
    std::cerr << "Unable to load input image " << fileInputImage.c_str() << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Create a mask of the correct dimension
  niftk::ImageToSurfaceFilter::Pointer surfaceFilter = niftk::ImageToSurfaceFilter::New();
  surfaceFilter->SetInput(inputImage);
  surfaceFilter->SetThreshold(threshold);
  surfaceFilter->SetSamplingRatio(samplingRatio);

  if (extractionType == 0)
  {
    surfaceFilter->SetSurfaceExtractionType(niftk::ImageToSurfaceFilter::StandardExtractor);
  }
  else if (extractionType == 1)
  {
    surfaceFilter->SetSurfaceExtractionType(niftk::ImageToSurfaceFilter::EnhancedCPUExtractor);
  }

  if (inputSmoothingType == 0)
  {
    surfaceFilter->SetPerformInputSmoothing(false);
  }
  else if (inputSmoothingType == 1)
  {
    surfaceFilter->SetPerformInputSmoothing(true);
    surfaceFilter->SetInputSmoothingType(niftk::ImageToSurfaceFilter::GaussianSmoothing);
  }
  else if (inputSmoothingType == 2)
  {
    surfaceFilter->SetPerformInputSmoothing(true);
    surfaceFilter->SetInputSmoothingType(niftk::ImageToSurfaceFilter::MedianSmoothing);
  }

  surfaceFilter->SetInputSmoothingIterations(inputSmoothingIterations);
  surfaceFilter->SetInputSmoothingRadius(inputSmoothingRadius);

  if (surfSmoothingType == 0)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(false);
  }
  else if (surfSmoothingType == 1)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(true);
    surfaceFilter->SetSurfaceSmoothingType(niftk::ImageToSurfaceFilter::TaubinSmoothing);
  }
  else if (surfSmoothingType == 2)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(true);
    surfaceFilter->SetSurfaceSmoothingType(niftk::ImageToSurfaceFilter::CurvatureNormalSmooth);
  }
  else if (surfSmoothingType == 3)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(true);
    surfaceFilter->SetSurfaceSmoothingType(niftk::ImageToSurfaceFilter::InverseEdgeLengthSmooth);
  }
  else if (surfSmoothingType == 4)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(true);
    surfaceFilter->SetSurfaceSmoothingType(niftk::ImageToSurfaceFilter::WindowedSincSmoothing);
  }
  else if (surfSmoothingType == 5)
  {
    surfaceFilter->SetPerformSurfaceSmoothing(true);
    surfaceFilter->SetSurfaceSmoothingType(niftk::ImageToSurfaceFilter::StandardVTKSmoothing);
  }

  surfaceFilter->SetSurfaceSmoothingIterations(surfSmoothingIterations);
  surfaceFilter->SetSurfaceSmoothingRadius(surfSmoothingRadius);

  if (decimationType == 0)
  {
    surfaceFilter->SetPerformSurfaceDecimation(false);
  }
  else if (decimationType == 1)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::DecimatePro);
  }
  else if (decimationType == 2)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::QuadricVTK);
  }
  else if (decimationType == 3)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::Quadric);
  }
  else if (decimationType == 4)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::QuadricTri);
  }
  else if (decimationType == 5)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::Melax);
  }
  else if (decimationType == 6)
  {
    surfaceFilter->SetPerformSurfaceDecimation(true);
    surfaceFilter->SetSurfaceDecimationType(niftk::ImageToSurfaceFilter::ShortestEdge);
  }

  surfaceFilter->SetTargetReduction(targetReduction);

  surfaceFilter->SetPerformSurfaceCleaning(useSurfClean);
  surfaceFilter->SetSurfaceCleaningThreshold(surfCleanThreshold);
  
  surfaceFilter->Update();
  mitk::Surface::Pointer surf = surfaceFilter->GetOutput();

  if (surf.IsNull())
  {
    std::cerr << "Unable to create surface." << std::endl;
    return EXIT_FAILURE;
  }

  // Will throw on failure.
  mitk::IOUtil::Save(surf, fileOutputPolyData);

  return EXIT_SUCCESS;
}
