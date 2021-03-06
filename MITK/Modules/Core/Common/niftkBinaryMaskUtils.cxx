/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkBinaryMaskUtils.h"
#include "niftkImageUtils.h"

#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <mitkExceptionMacro.h>

namespace niftk
{

//-----------------------------------------------------------------------------
bool IsBinaryMask(const mitk::Image::Pointer& input)
{
  if (input->GetNumberOfChannels() != 1)
  {
    return false;
  }

  if (input->GetPixelType().GetPixelType() != itk::ImageIOBase::SCALAR)
  {
    return false;
  }

  if (input->GetPixelType().GetComponentType() != itk::ImageIOBase::UCHAR)
  {
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
bool IsBinaryMask(const mitk::DataNode::Pointer& input)
{
  if (!niftk::IsImage(input))
  {
    return false;
  }

  bool isBinary = false;
  bool foundProperty = input->GetBoolProperty("binary", isBinary);
  if (!foundProperty || !isBinary)
  {
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
void ValidateBinaryMaskInputs(const mitk::Image::Pointer& input1,
                              const mitk::Image::Pointer& input2,
                              const mitk::Image::Pointer& output)
{
  if (input1.IsNull())
  {
    mitkThrow() << "Image input1 is NULL";
  }
  if (input2.IsNull())
  {
    mitkThrow() << "Image input2 is NULL";
  }
  if (output.IsNull())
  {
    mitkThrow() << "Image output is NULL";
  }
  if (niftk::GetNumberOfVoxels(input1) != niftk::GetNumberOfVoxels(input2))
  {
    mitkThrow() << "Input images have different number of voxels";
  }
  if (niftk::GetNumberOfVoxels(input1) != niftk::GetNumberOfVoxels(output))
  {
    mitkThrow() << "Input and output images have different number of voxels";
  }
  if (!niftk::IsBinaryMask(input1))
  {
    mitkThrow() << "Input 1 is not a binary mask.";
  }
  if (!niftk::IsBinaryMask(input2))
  {
    mitkThrow() << "Input 2 is not a binary mask.";
  }
  if (!niftk::IsBinaryMask(output))
  {
    mitkThrow() << "Output is not a binary mask.";
  }
}


//-----------------------------------------------------------------------------
void BinaryMaskAndOperator(const mitk::Image::Pointer& input1,
                           const mitk::Image::Pointer& input2,
                           mitk::Image::Pointer& output
                          )
{

  niftk::ValidateBinaryMaskInputs(input1, input2, output);

  mitk::ImageReadAccessor readAccess1(input1, input1->GetVolumeData(0));
  unsigned char* ip1 = static_cast<unsigned char*>(const_cast<void*>(readAccess1.GetData()));

  mitk::ImageReadAccessor readAccess2(input2, input2->GetVolumeData(0));
  unsigned char* ip2 = static_cast<unsigned char*>(const_cast<void*>(readAccess2.GetData()));

  mitk::ImageWriteAccessor writeAccess(output);
  unsigned char* op = static_cast<unsigned char*>(const_cast<void*>(writeAccess.GetData()));

  auto numberOfPixels = niftk::GetNumberOfVoxels(input1);

  for (unsigned int i = 0; i <= numberOfPixels; ++i)
  {
    if ((*ip1) & (*ip2))
    {
      *op = 255;
    }
    else
    {
      *op = 0;
    }

    ip1++;
    ip2++;
    op++;
  }
}


//-----------------------------------------------------------------------------
void BinaryMaskOrOperator(const mitk::Image::Pointer& input1,
                          const mitk::Image::Pointer& input2,
                          mitk::Image::Pointer& output
                         )
{
  niftk::ValidateBinaryMaskInputs(input1, input2, output);

  mitk::ImageReadAccessor readAccess1(input1, input1->GetVolumeData(0));
  unsigned char* ip1 = static_cast<unsigned char*>(const_cast<void*>(readAccess1.GetData()));

  mitk::ImageReadAccessor readAccess2(input2, input2->GetVolumeData(0));
  unsigned char* ip2 = static_cast<unsigned char*>(const_cast<void*>(readAccess2.GetData()));

  mitk::ImageWriteAccessor writeAccess(output);
  unsigned char* op = static_cast<unsigned char*>(const_cast<void*>(writeAccess.GetData()));

  auto numberOfPixels = niftk::GetNumberOfVoxels(input1);

  for (unsigned int i = 0; i <= numberOfPixels; ++i)
  {
    if ((*ip1) | (*ip2))
    {
      *op = 255;
    }
    else
    {
      *op = 0;
    }

    ip1++;
    ip2++;
    op++;
  }
}


//-----------------------------------------------------------------------------
void BinaryMaskCopyOperator(const mitk::Image::Pointer& input,
                            mitk::Image::Pointer& output
                           )
{

  if (input.IsNull())
  {
    mitkThrow() << "Input image is NULL";
  }
  if (output.IsNull())
  {
    mitkThrow() << "Output image is NULL";
  }
  if (niftk::GetNumberOfVoxels(input) != niftk::GetNumberOfVoxels(output))
  {
    mitkThrow() << "Input and output images have different number of voxels";
  }
  if (!niftk::IsBinaryMask(input))
  {
    mitkThrow() << "Input is not a binary mask.";
  }
  if (!niftk::IsBinaryMask(output))
  {
    mitkThrow() << "Output is not a binary mask.";
  }

  mitk::ImageReadAccessor readAccess(input, input->GetVolumeData(0));
  unsigned char* ip = static_cast<unsigned char*>(const_cast<void*>(readAccess.GetData()));

  mitk::ImageWriteAccessor writeAccess(output);
  unsigned char* op = static_cast<unsigned char*>(const_cast<void*>(writeAccess.GetData()));

  auto numberOfPixels = niftk::GetNumberOfVoxels(input);

  std::memcpy(op, ip, numberOfPixels);
}

} // end namespace
