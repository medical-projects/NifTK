/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.
 
 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-11-21 14:43:44 +0000 (Mon, 21 Nov 2011) $
 Revision          : $Revision: 7828 $
 Last modified by  : $Author: kkl $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include "itkLogHelper.h"
#include "itkCropImageFilter.h"
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkNumericTraits.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionIteratorWithIndex.h"

#include "niftkCropImageCLP.h"

/*!
 * \file niftkCrop.cxx
 * \page niftkCrop
 * \section niftkCropSummary Crops the input image using a mask and/or a voxel-wise bounding box.
 *
 * \li Dimensions: 3
 * \li Pixel type: Scalars only, of type short.
 *
 * \section niftkCropCavear Caveats
 * \li File sizes not checked.
 * \li Image headers not checked. By "voxel by voxel basis" we mean that the image geometry, origin, orientation is not checked.
 */

void Usage(char *exec)
  {
    niftk::itkLogHelper::PrintCommandLineHeader(std::cout);
    std::cout << "  " << std::endl;
    std::cout << "  Crops the input image using the mask." << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "  " << exec << " -i inputFileName -m maskFileName -o outputFileName" << std::endl;
    std::cout << "  " << std::endl;
    return;
  }

/**
 * \brief Takes mask and target and crops target where mask is non zero.
 */
int main(int argc, char** argv)
{

  const   unsigned int Dimension = 3;
  typedef short        PixelType;

  typedef itk::Image< PixelType, Dimension > ImageType;   
  typedef itk::ImageFileReader< ImageType >  InputImageReaderType;
  typedef itk::ImageFileWriter< ImageType >  OutputImageWriterType;

  typedef itk::CropImageFilter< ImageType, ImageType > CropFilterType;

  typedef itk::ImageRegionIteratorWithIndex< ImageType > IteratorType;    

  ImageType::Pointer inImage = 0;
  ImageType::Pointer maskImage = 0;

  ImageType::IndexType idx;

  float startCoord[4];
  float endCoord[4];


  // Validate command line args
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~

  PARSE_ARGS;

  if ( fileInputImage.length() == 0 || fileOutputImage.length() == 0 )
  {
    commandLine.getOutput()->usage(commandLine);
    return EXIT_FAILURE;
  }

  startCoord[0] = sx;
  startCoord[1] = sy;
  startCoord[2] = sz;
  startCoord[3] = st;

  endCoord[0] = ex;
  endCoord[1] = ey;
  endCoord[2] = ez;
  endCoord[3] = et;


  // Read the input image
  // ~~~~~~~~~~~~~~~~~~~~

  InputImageReaderType::Pointer imageReader = InputImageReaderType::New();
  
  imageReader->SetFileName( fileInputImage );
  
  try
  {
    std::cout << "Reading input image: " << fileInputImage << std::endl; 
    imageReader->Update(); 
  }
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ERROR: Failed to read the input image: " << err << std::endl; 
    return EXIT_FAILURE;
  }                
  
  inImage = imageReader->GetOutput();
  inImage->DisconnectPipeline();


  // Calculate the extent of the bounding specified (the whole image by default)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  unsigned int i;
  ImageType::IndexType idxStart, idxEnd;
  ImageType::SizeType size = inImage->GetLargestPossibleRegion().GetSize();
  
  // Convert the indices from mm?

  if ( flgIndicesInMM ) 
  {
    ImageType::PointType ptStart, ptEnd;

    for ( i=0; i<Dimension; i++ )
    {
      ptStart[i] = startCoord[i];
      ptEnd[i]   =   endCoord[i];
    }

    inImage->TransformPhysicalPointToIndex( ptStart, idxStart );
    inImage->TransformPhysicalPointToIndex(   ptEnd,   idxEnd );
  }
  else
  {
    for ( i=0; i<Dimension; i++ )
    { 
      if ( startCoord[i] < 0 )
	idxStart[i] = 0;
      else if ( startCoord[i] > size[i] - 1 )
	idxStart[i] = size[i] - 1;
      else
	idxStart[i] = static_cast<int>( startCoord[i] );
      
      if ( endCoord[i] < 0 )
	idxEnd[i] = 0;
      else if ( endCoord[i] > size[i] - 1 )
	idxEnd[i] = size[i] - 1;
      else
	idxEnd[i]   = static_cast<int>( endCoord[i] );
    }
  }

  // Clip the indices to the image size
  
  unsigned int idxTmp;

  for ( i=0; i<Dimension; i++ )
  { 
    if ( idxStart[i] < 0 ) idxStart[i] = 0;
    if ( idxStart[i] > static_cast<int>( size[i] - 1 ) ) idxStart[i] = size[i] - 1;
  
    if ( idxEnd[i] < 0 ) idxEnd[i] = 0;
    if ( idxEnd[i] > static_cast<int>( size[i] - 1 ) ) idxEnd[i] = size[i] - 1;
  
    if ( idxStart[i] > idxEnd[i] ) {
      idxTmp = idxEnd[i];
      idxEnd[i] = idxStart[i];
      idxStart[i] = idxTmp;
    }

    size[i] = idxEnd[i] - idxStart[i];
  }

  std::cout << "Crop bounding box:" << std::endl
	    << "   from: " << idxStart << std::endl
	    << "   to:   " << idxEnd << std::endl
	    << "   size: " << size << std::endl;
  

  // Read the input mask and calculate the combined bounding box
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  if ( fileMaskImage.length() )
  {
    InputImageReaderType::Pointer maskReader = InputImageReaderType::New();

    maskReader->SetFileName( fileMaskImage );
  
    try
    {
      std::cout << "Reading mask image: " << fileMaskImage << std::endl; 
      maskReader->Update(); 
    }
    catch( itk::ExceptionObject & err ) 
    { 
      std::cerr << "ERROR: Failed to read the mask image: " << err << std::endl; 
      return EXIT_FAILURE;
    }                

    maskImage = maskReader->GetOutput();
    maskImage->DisconnectPipeline();


    // The mask and the bounding box combine...?

    if ( flgCombineMaskAndBoundingBoxViaUnion )
    {
      ImageType::RegionType region = inImage->GetLargestPossibleRegion();
      
      IteratorType inputIterator( maskImage, region );
      
      for ( inputIterator.GoToBegin(); 
	    ! inputIterator.IsAtEnd();
	    ++inputIterator )
      {
	if ( inputIterator.Get() ) {
	  idx = inputIterator.GetIndex();
	  
	  for ( i=0; i<Dimension; i++ )
	  {
	    if ( idx[i] < idxStart[i] ) 
	      idxStart[i] = idx[i];
	      
	    if ( idx[i] > idxEnd[i] )
	      idxEnd[i] = idx[i];
	  }
	}
      }
    }
  

    // ...or only where the mask and bounding box overlap?
    
    else
    {
      ImageType::RegionType region = inImage->GetLargestPossibleRegion();
      
      region.SetSize( size );
      region.SetIndex( idxStart );

      idx = idxStart;
      idxStart = idxEnd;
      idxEnd = idx;

      IteratorType inputIterator( maskImage, region );
      
      for ( inputIterator.GoToBegin(); 
	    ! inputIterator.IsAtEnd();
	    ++inputIterator )
      {
	if ( inputIterator.Get( ) )
	{
	  idx = inputIterator.GetIndex();
	
	  for ( i=0; i<Dimension; i++ )
	  {
	    if ( idx[i] < idxStart[i] ) 
	      idxStart[i] = idx[i];
	      
	    if ( idx[i] > idxEnd[i] )
	      idxEnd[i] = idx[i];
	  }
	}
      }
    }
  }



  // Crop the image

  CropFilterType::Pointer filter = CropFilterType::New();  

  ImageType::SizeType lowerSize, upperSize;
  
  for ( i=0; i<Dimension; i++ )
    lowerSize[i] = idxStart[i];

  filter->SetLowerBoundaryCropSize( lowerSize );


  for ( i=0; i<Dimension; i++ )
    upperSize[i] = inImage->GetLargestPossibleRegion().GetSize()[i] - idxEnd[i];

  filter->SetUpperBoundaryCropSize( upperSize );


  filter->SetInput( inImage );



  // Write the cropped image to a file

  OutputImageWriterType::Pointer imageWriter = OutputImageWriterType::New();

  imageWriter->SetFileName( fileOutputImage );
  imageWriter->SetInput( filter->GetOutput() );
  
  try
  {
    imageWriter->Update(); 
  }
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ERROR: Failed to crop and write the image to a file." << err << std::endl; 
    return EXIT_FAILURE;
  }                

  return EXIT_SUCCESS; 
}
