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

#include "mitkMIDASImageUtils.h"

#include <mitkImageAccessByItk.h>
#include <mitkITKImageImport.h>
#include <itkImage.h>
#include <itkImageRegionConstIterator.h>
#include <mitkPositionEvent.h>
#include <mitkStateEvent.h>
#include <mitkInteractionConst.h>

namespace mitk
{

//-----------------------------------------------------------------------------
template<typename TPixel, unsigned int VImageDimension>
void
ITKGetAsAcquiredOrientation(
  itk::Image<TPixel, VImageDimension>* itkImage,
  MIDASOrientation &outputOrientation
)
{
  typedef itk::Image<TPixel, VImageDimension> ImageType;

  typename itk::SpatialOrientationAdapter adaptor;
  typename itk::SpatialOrientation::ValidCoordinateOrientationFlags orientation;
  orientation = adaptor.FromDirectionCosines(itkImage->GetDirection());
  std::string orientationString = itk::ConvertSpatialOrientationToString(orientation);

  if (orientationString[0] == 'L' || orientationString[0] == 'R')
  {
    if (orientationString[1] == 'A' || orientationString[1] == 'P')
    {
      outputOrientation = MIDAS_ORIENTATION_AXIAL;
    }
    else
    {
      outputOrientation = MIDAS_ORIENTATION_CORONAL;
    }
  }
  else if (orientationString[0] == 'A' || orientationString[0] == 'P')
  {
    if (orientationString[1] == 'L' || orientationString[1] == 'R')
    {
      outputOrientation = MIDAS_ORIENTATION_AXIAL;
    }
    else
    {
      outputOrientation = MIDAS_ORIENTATION_SAGITTAL;
    }
  }
  else if (orientationString[0] == 'S' || orientationString[0] == 'I')
  {
    if (orientationString[1] == 'L' || orientationString[1] == 'R')
    {
      outputOrientation = MIDAS_ORIENTATION_CORONAL;
    }
    else
    {
      outputOrientation = MIDAS_ORIENTATION_SAGITTAL;
    }
  }
}


//-----------------------------------------------------------------------------
MIDASView GetAsAcquiredView(const MIDASView& defaultView, const mitk::Image* image)
{
  MIDASView view = defaultView;
  if (image != NULL)
  {
    // "As Acquired" means you take the orientation of the XY plane
    // in the original image data, so we switch to ITK to work it out.
    MIDASOrientation orientation = MIDAS_ORIENTATION_UNKNOWN;
    int dimensions = image->GetDimension();
    switch(dimensions)
    {
    case 3:
      AccessFixedDimensionByItk_n(image, ITKGetAsAcquiredOrientation, 3, (orientation));
      break;
    default:
      MITK_ERROR << "During GetAsAcquiredView, unsupported number of dimensions:" << dimensions << std::endl;
    }

    if (orientation == MIDAS_ORIENTATION_AXIAL)
    {
      view = MIDAS_VIEW_AXIAL;
    }
    else if (orientation == MIDAS_ORIENTATION_SAGITTAL)
    {
      view = MIDAS_VIEW_SAGITTAL;
    }
    else if (orientation == MIDAS_ORIENTATION_CORONAL)
    {
      view = MIDAS_VIEW_CORONAL;
    }
    else
    {
      MITK_ERROR << "GetAsAcquiredView defaulting to view=" << view << std::endl;
    }
  }
  return view;
}


//-----------------------------------------------------------------------------
bool IsImage(const mitk::DataNode* node)
{
  bool result = false;
  if (node != NULL && dynamic_cast<mitk::Image*>(node->GetData()))
  {
    result = true;
  }
  return true;
}


//-----------------------------------------------------------------------------
template<typename TPixel, unsigned int VImageDimension>
void
ITKImagesHaveEqualIntensities(
    const itk::Image<TPixel, VImageDimension>* itkImage,
    const mitk::Image* image2,
    bool &output
    )
{
  output = true;

  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typedef mitk::ImageToItk< ImageType > ImageToItkType;

  typename ImageToItkType::Pointer itkImage2 = ImageToItkType::New();
  itkImage2->SetInput(image2);
  itkImage2->Update();

  typename ImageType::ConstPointer im1 = itkImage;
  typename ImageType::ConstPointer im2 = itkImage2->GetOutput();

  itk::ImageRegionConstIterator<ImageType> iter1(im1, im1->GetLargestPossibleRegion());
  itk::ImageRegionConstIterator<ImageType> iter2(im2, im2->GetLargestPossibleRegion());
  for (iter1.GoToBegin(), iter2.GoToBegin();
      !iter1.IsAtEnd() && !iter2.IsAtEnd();
      ++iter1, ++iter2
      )
  {
    if (iter1.Get() != iter2.Get())
    {
      output = false;
      return;
    }
  }
}


//-----------------------------------------------------------------------------
bool ImagesHaveEqualIntensities(const mitk::Image* image1, const mitk::Image* image2)
{
  bool result = false;

  if (image1 != NULL && image2 != NULL)
  {
    try
    {
      int dimensions = image1->GetDimension();
      switch(dimensions)
      {
      case 2:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveEqualIntensities, 2, (image2, result));
        break;
      case 3:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveEqualIntensities, 3, (image2, result));
        break;
      case 4:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveEqualIntensities, 4, (image2, result));
        break;
      default:
        MITK_ERROR << "During ImagesHaveEqualIntensities, unsupported number of dimensions:" << dimensions << std::endl;
      }
    }
    catch (const mitk::AccessByItkException &e)
    {
      MITK_ERROR << "ImagesHaveEqualIntensities: AccessFixedDimensionByItk_n failed to check equality due to." << e.what() << std::endl;
    }
  }

  return result;
}


//-----------------------------------------------------------------------------
template<typename TPixel, unsigned int VImageDimension>
void
ITKImagesHaveSameSpatialExtent(
    const itk::Image<TPixel, VImageDimension>* itkImage,
    const mitk::Image* image2,
    bool &output
    )
{
  output = true;

  typedef itk::Image<TPixel, VImageDimension> ImageType;
  typedef mitk::ImageToItk< ImageType > ImageToItkType;

  typename ImageToItkType::Pointer itkImage2 = ImageToItkType::New();
  itkImage2->SetInput(image2);
  itkImage2->Update();

  typename ImageType::ConstPointer im1 = itkImage;
  typename ImageType::ConstPointer im2 = itkImage2->GetOutput();

  if (im1->GetLargestPossibleRegion() != im2->GetLargestPossibleRegion())
  {
    output = false;
    return;
  }

  if (im1->GetOrigin() != im2->GetOrigin())
  {
    output = false;
    return;
  }

  if (im1->GetSpacing() != im2->GetSpacing())
  {
    output = false;
    return;
  }

  if (im1->GetDirection() != im2->GetDirection())
  {
    output = false;
    return;
  }
}


//-----------------------------------------------------------------------------
bool ImagesHaveSameSpatialExtent(const mitk::Image* image1, const mitk::Image* image2)
{
  bool result = false;

  if (image1 != NULL && image2 != NULL)
  {
    try
    {
      int dimensions = image1->GetDimension();
      switch(dimensions)
      {
      case 2:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveSameSpatialExtent, 2, (image2, result));
        break;
      case 3:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveSameSpatialExtent, 3, (image2, result));
        break;
      case 4:
        AccessFixedDimensionByItk_n(image1, ITKImagesHaveSameSpatialExtent, 4, (image2, result));
        break;
      default:
        MITK_ERROR << "During ImagesHaveSameSpatialExtent, unsupported number of dimensions:" << dimensions << std::endl;
      }
    }
    catch (const mitk::AccessByItkException &e)
    {
      MITK_ERROR << "ImagesHaveSameSpatialExtent: AccessFixedDimensionByItk_n failed to check equality due to." << e.what() << std::endl;
    }
  }

  return result;
}


//-----------------------------------------------------------------------------
template<typename TPixel, unsigned int VImageDimension>
void
ITKFillImage(
    itk::Image<TPixel, VImageDimension>* itkImage,
    float &value
    )
{
  itkImage->FillBuffer((TPixel)value);
}


//-----------------------------------------------------------------------------
void FillImage(mitk::Image* image, float value)
{
  if (image != NULL)
  {
    try
    {
      int dimensions = image->GetDimension();
      switch(dimensions)
      {
      case 2:
        AccessFixedDimensionByItk_n(image, ITKFillImage, 2, (value));
        break;
      case 3:
        AccessFixedDimensionByItk_n(image, ITKFillImage, 3, (value));
        break;
      case 4:
        AccessFixedDimensionByItk_n(image, ITKFillImage, 4, (value));
        break;
      default:
        MITK_ERROR << "During FillImage, unsupported number of dimensions:" << dimensions << std::endl;
      }
    }
    catch (const mitk::AccessByItkException &e)
    {
      MITK_ERROR << "FillImage: AccessFixedDimensionByItk_n failed to fill images due to." << e.what() << std::endl;
    }
  }
}


//-----------------------------------------------------------------------------
template<typename TPixel, unsigned int VImageDimension>
void
ITKCountBetweenThreshold(
    itk::Image<TPixel, VImageDimension>* itkImage,
    float &lower,
    float &upper,
    unsigned long int &outputCount
    )
{
  typedef itk::Image<TPixel, VImageDimension> ImageType;
  itk::ImageRegionConstIterator<ImageType> iter(itkImage, itkImage->GetLargestPossibleRegion());
  outputCount = 0;
  TPixel value;
  for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter)
  {
    value = iter.Get();
    if (value >= lower && value <= upper)
    {
      outputCount++;
    }
  }
}


//-----------------------------------------------------------------------------
/**
 * \brief Simply iterates through a whole image, counting how many intensity values are >= lower and <= upper.
 * \param lower A lower threshold for intensity values
 * \param upper An upper threshold for intensity values
 * \return unsigned long int The number of voxels.
 */
NIFTKMITKEXT_EXPORT unsigned long int CountBetweenThreshold(mitk::Image* image, float lower, float upper)
{
  unsigned long int counter = 0;

  if (image != NULL)
  {
    try
    {
      int dimensions = image->GetDimension();
      switch(dimensions)
      {
      case 2:
        AccessFixedDimensionByItk_n(image, ITKCountBetweenThreshold, 2, (lower, upper, counter));
        break;
      case 3:
        AccessFixedDimensionByItk_n(image, ITKCountBetweenThreshold, 3, (lower, upper, counter));
        break;
      case 4:
        AccessFixedDimensionByItk_n(image, ITKCountBetweenThreshold, 4, (lower, upper, counter));
        break;
      default:
        MITK_ERROR << "During CountBetweenThreshold, unsupported number of dimensions:" << dimensions << std::endl;
      }
    }
    catch (const mitk::AccessByItkException &e)
    {
      MITK_ERROR << "CountBetweenThreshold: AccessFixedDimensionByItk_n failed to count voxels due to." << e.what() << std::endl;
    }
  }

  return counter;
}


//-----------------------------------------------------------------------------
unsigned long int GetNumberOfVoxels(mitk::Image* image)
{
  unsigned long int counter = 0;

  if (image != NULL)
  {
    counter = 1;
    for (unsigned int i = 0; i < image->GetDimension(); i++)
    {
      counter *= image->GetDimension(i);
    }
  }
  return counter;
}


//-----------------------------------------------------------------------------
mitk::Point3D GetMiddlePointInVoxels(mitk::Image* image)
{
  mitk::Point3D voxelIndex;
  voxelIndex[0] = (int)(image->GetDimension(0)/2.0);
  voxelIndex[1] = (int)(image->GetDimension(1)/2.0);
  voxelIndex[2] = (int)(image->GetDimension(2)/2.0);
  return voxelIndex;
}

//-----------------------------------------------------------------------------
mitk::PositionEvent GeneratePositionEvent(mitk::BaseRenderer* renderer, mitk::Image* image, mitk::Point3D voxelLocation)
{
  mitk::Point2D point2D;
  point2D[0] = 0;
  point2D[1] = 0;

  mitk::Point3D millimetreCoordinate;
  image->GetGeometry()->IndexToWorld(voxelLocation, millimetreCoordinate);

  mitk::PositionEvent event( renderer, 0, 0, 0, mitk::Key_unknown, point2D, millimetreCoordinate );
  return event;
}

} // end namespace

