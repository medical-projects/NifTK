/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include <iostream>
#include <niftkLogHelper.h>
#include <niftkConversionUtils.h>
#include <itkBilateralImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkNifTKImageIOFactory.h>
#include <itkCommandLineHelper.h>

/*!
 * \file niftkBilateralImageFilter.cxx
 * \page niftkBilateralImageFilter
 * \section niftkBilateralImageFilterSummary Runs ITK BilateralImageFilter on a 3D image to do bilteral filter.
 *
 * Runs ITK BilateralImageFilter to do bilteral filter. 
 * \li Dimensions: 3
 * \li Pixel type: Scalars only, of unsigned char, char, unsigned short, short, unsigned int, int, unsigned long, long, float, double
 *
 * \section niftkBilteralImageFilterCaveat Caveats
 */
void StartUsage(char *name)
{
  niftk::LogHelper::PrintCommandLineHeader(std::cout);
  std::cout << "  " << std::endl;
  std::cout << "  Bilteral filter from ITK. See itk::BilateralImageFilter." << std::endl;
  std::cout << "  " << std::endl; 
  std::cout << "  -i <input> -o <output> -ds <domain/spatial sigma> -rs <range/intensity sigma>" << std::endl; 
  std::cout << "  " << std::endl;
}

  

int main(int argc, char* argv[])
{
  itk::NifTKImageIOFactory::Initialize();

  std::string inputImageName; 
  std::string outputImageName; 
  double domainSigma = 0.0; 
  double rangeSigma = 0.0; 
  
  if (argc < 9)
  {
    StartUsage(argv[0]); 
    return EXIT_FAILURE; 
  }

  

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-help")==0 || strcmp(argv[i], "-Help")==0 || strcmp(argv[i], "-HELP")==0 || strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--h")==0)
    {
      StartUsage(argv[0]);
      return -1;
    }
    else if (strcmp(argv[i], "-i") == 0)
    {
      inputImageName = argv[++i];
      std::cout << "Set -i=" << inputImageName<< std::endl;
    }
    else if (strcmp(argv[i], "-o") == 0)
    {
      outputImageName = argv[++i];
      std::cout << "Set -i=" << outputImageName<< std::endl;
    }
    else if (strcmp(argv[i], "-ds") == 0)
    {
      domainSigma = atof(argv[++i]);
      std::cout << "Set -ds=" << niftk::ConvertToString(domainSigma)<< std::endl;
    }
    else if (strcmp(argv[i], "-rs") == 0)
    {
      rangeSigma = atof(argv[++i]);
      std::cout << "Set -rs=" << niftk::ConvertToString(rangeSigma)<< std::endl;
    }
    else
    {
      std::cerr << argv[0] << ":\tParameter " << argv[i] << " unknown." << std::endl;
      StartUsage(argv[0]); 
      return -1;
    }
  }
  
  const unsigned int Dimension = 3;
  int dims = itk::PeekAtImageDimension(inputImageName);
  // Define the dimension of the images

  if (dims != Dimension)
  {
    std::cerr << "Unsupported image dimension." << std::endl;
    return EXIT_FAILURE;
  }

  typedef short PixelType;
  typedef itk::Image< PixelType, Dimension > ImageType;
  typedef itk::ImageFileReader< ImageType > ImageReaderType;
  typedef itk::ImageFileWriter< ImageType > ImageWriterType;
  typedef itk::BilateralImageFilter<ImageType, ImageType> BilateralImageFilterType;

  try
  {
    ImageReaderType::Pointer reader = ImageReaderType::New(); 
    reader->SetFileName(inputImageName); 
    reader->Update(); 
    
    BilateralImageFilterType::Pointer filter = BilateralImageFilterType::New();
    filter->SetDomainSigma(domainSigma);
    filter->SetRangeSigma(rangeSigma);
    filter->SetInput(reader->GetOutput()); 
    
    ImageWriterType::Pointer writer = ImageWriterType::New(); 
    writer->SetInput(filter->GetOutput()); 
    writer->SetFileName(outputImageName); 
    writer->Update(); 
  }
  catch(itk::ExceptionObject &err)
  {
    (&err)->Print(std::cerr);
    return EXIT_FAILURE;
  } 
  return EXIT_SUCCESS;   
}


