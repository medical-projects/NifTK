/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

  =============================================================================*/

#include <iomanip> 

#include <niftkConversionUtils.h>

#include <itkLogHelper.h>
#include <itkImage.h>
#include <itkCommandLineHelper.h>

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

#include <itkMammogramMaskSegmentationImageFilter.h>

#include <niftkMammogramMaskSegmentationCLP.h>

/*!
 * \file niftkMammogramMaskSegmentation.cxx
 * \page niftkMammogramMaskSegmentation
 * \section niftkMammogramMaskSegmentationSummary Segments a mammogram generating a binary mask corresponding to the breast area.
 *
 * This program uses ITK ImageFileReader to load an image, and then uses MammogramMaskSegmentationImageFilter to segment the breast reagion before writing the output with ITK ImageFileWriter.
 *
 * \li Dimensions: 2.
 * \li Pixel type: Scalars only of unsigned char, char, unsigned short, short, unsigned int, int, unsigned long, long, float and double.
 *
 * \section niftkMammogramMaskSegmentationCaveats Caveats
 * \li None
 */

struct arguments
{
  bool flgVerbose;
  bool flgDebug;
  bool flgApplyMaskToImage;
  
  std::string inputImage;
  std::string outputImage;  
  
  arguments() {
    flgVerbose = false;
    flgDebug = false;
    flgApplyMaskToImage = false;
  }

};


template <int Dimension, class InputPixelType> 
int DoMain(arguments args)
{  
  unsigned int i;

  typedef itk::Image< InputPixelType, Dimension > InputImageType;   

  typedef unsigned char MaskPixelType;
  typedef itk::Image< MaskPixelType, Dimension > MaskImageType;   

  typedef itk::ImageFileReader< InputImageType > InputImageReaderType;

  typedef itk::MammogramMaskSegmentationImageFilter<InputImageType, MaskImageType> MammogramMaskSegmentationImageFilterType;


  // Check that the input is 2D
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~

  int dims = itk::PeekAtImageDimensionFromSizeInVoxels(args.inputImage);
  if (dims != 2)
  {
    std::cout << "ERROR: Unsupported image dimension" << std::endl;
    return EXIT_FAILURE;
  }
  else if (dims == 2)
  {
    std::cout << "Input is 2D" << std::endl;
  }


  // Read the input image
  // ~~~~~~~~~~~~~~~~~~~~

  typename InputImageReaderType::Pointer imageReader = InputImageReaderType::New();
  imageReader->SetFileName( args.inputImage );

  try {
    imageReader->Update();
  }
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ERROR: Failed to read image: " 
              << args.inputImage << std::endl
              << err << std::endl; 
    return EXIT_FAILURE;
  }                

  if ( args.flgDebug )
  {
    imageReader->GetOutput()->Print( std::cout );
  }

  typename InputImageType::Pointer image = imageReader->GetOutput();

  image->DisconnectPipeline();


  // Create the segmentation filter
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  typename MammogramMaskSegmentationImageFilterType::Pointer filter = MammogramMaskSegmentationImageFilterType::New();

  filter->SetInput( image );

  filter->SetDebug(   args.flgDebug );
  filter->SetVerbose( args.flgVerbose );

  try {
    filter->Update();
  }
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ERROR: Failed to segment image" << std::endl
              << err << std::endl; 
    return EXIT_FAILURE;
  }                

  typename MaskImageType::Pointer mask = filter->GetOutput();

  mask->DisconnectPipeline();


  // Apply the mask to the image?
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  if ( args.flgApplyMaskToImage )
  {

    typename itk::ImageRegionConstIterator< MaskImageType > 
      inputIterator( mask, mask->GetLargestPossibleRegion());

    typename itk::ImageRegionIterator< InputImageType > 
      outputIterator(image, image->GetLargestPossibleRegion());
        
    for ( inputIterator.GoToBegin(), outputIterator.GoToBegin();
          ! inputIterator.IsAtEnd();
          ++inputIterator, ++outputIterator )
    {
      if ( ! inputIterator.Get() )
        outputIterator.Set( 0 );
    }


    typedef itk::ImageFileWriter< InputImageType > InputImageWriterType;

    typename InputImageWriterType::Pointer imageWriter = InputImageWriterType::New();

    imageWriter->SetFileName(args.outputImage);
    imageWriter->SetInput( image );
  
    try
    {
      imageWriter->Update(); 
    }
    catch( itk::ExceptionObject & err ) 
    { 
      std::cerr << "Failed: " << err << std::endl; 
      return EXIT_FAILURE;
    }       
  }

  else
  {
    typedef itk::ImageFileWriter< MaskImageType > MaskImageWriterType;

    typename MaskImageWriterType::Pointer imageWriter = MaskImageWriterType::New();

    imageWriter->SetFileName(args.outputImage);
    imageWriter->SetInput( mask );
  
    try
    {
      imageWriter->Update(); 
    }
    catch( itk::ExceptionObject & err ) 
    { 
      std::cerr << "Failed: " << err << std::endl; 
      return EXIT_FAILURE;
    }       
  }         

  return EXIT_SUCCESS;
}


/**
 * \brief Takes the input and segments it using itk::MammogramMaskSegmentationImageFilter
 */

int main(int argc, char** argv)
{

  // Parse the command line arguments
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  PARSE_ARGS;

  // To pass around command line args
  struct arguments args;

  args.flgVerbose = flgVerbose;
  args.flgDebug = flgDebug;
  args.flgApplyMaskToImage = flgApplyMaskToImage;

  args.inputImage=inputImage.c_str();
  args.outputImage=outputImage.c_str();

  std::cout << "Input image:  " << args.inputImage << std::endl
            << "Output image: " << args.outputImage << std::endl;

  // Validate command line args

  if ( (  args.inputImage.length() == 0 ) ||
       ( args.outputImage.length() == 0 ) )
  {
    return EXIT_FAILURE;
  }


  int dims = itk::PeekAtImageDimensionFromSizeInVoxels(args.inputImage);
  if (dims != 2)
  {
    std::cout << "ERROR: Input image must be 2D" << std::endl;
    return EXIT_FAILURE;
  }
   
  int result;

  switch (itk::PeekAtComponentType(args.inputImage))
  {

  case itk::ImageIOBase::UCHAR:
    std::cout << "Input is UNSIGNED CHAR" << std::endl;
    result = DoMain<2, unsigned char>(args);  
    break;

  case itk::ImageIOBase::CHAR:
    std::cout << "Input is CHAR" << std::endl;
    result = DoMain<2, char>(args);  
    break;

  case itk::ImageIOBase::USHORT:
    std::cout << "Input is UNSIGNED SHORT" << std::endl;
    result = DoMain<2, unsigned short>(args);  
    break;

  case itk::ImageIOBase::SHORT:
    std::cout << "Input is SHORT" << std::endl;
    result = DoMain<2, short>(args);  
    break;

  case itk::ImageIOBase::UINT:
    std::cout << "Input is UNSIGNED INT" << std::endl;
    result = DoMain<2, unsigned int>(args);  
    break;

  case itk::ImageIOBase::INT:
    std::cout << "Input is INT" << std::endl;
    result = DoMain<2, int>(args);  
    break;

  case itk::ImageIOBase::ULONG:
    std::cout << "Input is UNSIGNED LONG" << std::endl;
    result = DoMain<2, unsigned long>(args);  
    break;

  case itk::ImageIOBase::LONG:
    std::cout << "Input is LONG" << std::endl;
    result = DoMain<2, long>(args);  
    break;

  case itk::ImageIOBase::FLOAT:
    std::cout << "Input is FLOAT" << std::endl;
    result = DoMain<2, float>(args);  
    break;

  case itk::ImageIOBase::DOUBLE:
    std::cout << "Input is DOUBLE" << std::endl;
    result = DoMain<2, double>(args);  
    break;

  default:
    std::cerr << "ERROR: non standard pixel format" << std::endl;
    return EXIT_FAILURE;
  }

  return result;
}
