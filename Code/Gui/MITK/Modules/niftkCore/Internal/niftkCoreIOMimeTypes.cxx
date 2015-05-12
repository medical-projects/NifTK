/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "niftkCoreIOMimeTypes.h"
#include <mitkIOMimeTypes.h>

namespace niftk
{

//-----------------------------------------------------------------------------
std::vector<mitk::CustomMimeType*> CoreIOMimeTypes::Get()
{
  std::vector<mitk::CustomMimeType*> mimeTypes;

  // order matters here (descending rank for mime types)
  mimeTypes.push_back(TRANSFORM4X4_MIMETYPE().Clone());
  mimeTypes.push_back(PNM_MIMETYPE().Clone());

  return mimeTypes;
}


//-----------------------------------------------------------------------------
mitk::CustomMimeType CoreIOMimeTypes::TRANSFORM4X4_MIMETYPE()
{
  std::string category = "4x4 Transform File";

  mitk::CustomMimeType mimeType(TRANSFORM4X4_MIMETYPE_NAME());
  mimeType.SetCategory(category);
  mimeType.SetComment("4x4 Transform");
  mimeType.AddExtension("4x4");
  return mimeType;
}


//-----------------------------------------------------------------------------
std::string CoreIOMimeTypes::TRANSFORM4X4_MIMETYPE_NAME()
{
  static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".4x4";
  return name;
}


//-----------------------------------------------------------------------------
std::string CoreIOMimeTypes::TRANSFORM4X4_MIMETYPE_DESCRIPTION()
{
  static std::string description = "4x4 Transforms";
  return description;
}


// ------------------------------ Netpbm image formats ----------------------------------
// Generic Mime type

mitk::CustomMimeType CoreIOMimeTypes::PNM_MIMETYPE()
{
  mitk::CustomMimeType mimeType(PNM_MIMETYPE_NAME());
  std::string category = "PNM Image File";
  mimeType.SetComment("Netpbm format image");
  mimeType.SetCategory(category);
  mimeType.AddExtension("ppm");
  mimeType.AddExtension("pbm");
  mimeType.AddExtension("pgm");
  
  return mimeType;
}

// Specific Mime types

mitk::CustomMimeType CoreIOMimeTypes::PBM_MIMETYPE()
{
  mitk::CustomMimeType mimeType(PBM_MIMETYPE_NAME());
  std::string category = "PBM Image File";
  mimeType.SetComment("Portable bitmap format image");
  mimeType.SetCategory(category);
  mimeType.AddExtension("PBM");
  mimeType.AddExtension("pbm");
  return mimeType;
}

mitk::CustomMimeType CoreIOMimeTypes::PGM_MIMETYPE()
{
  mitk::CustomMimeType mimeType(PGM_MIMETYPE_NAME());
  std::string category = "PGM Image File";
  mimeType.SetComment("Portable greymap format image");
  mimeType.SetCategory(category);
  mimeType.AddExtension("PGM");
  mimeType.AddExtension("pgm");
  return mimeType;
}

mitk::CustomMimeType CoreIOMimeTypes::PPM_MIMETYPE()
{
  mitk::CustomMimeType mimeType(PPM_MIMETYPE_NAME());
  std::string category = "PPM Image File";
  mimeType.SetComment("Portable pixelmap format image");
  mimeType.SetCategory(category);
  mimeType.AddExtension("PPM");
  mimeType.AddExtension("ppm");
  return mimeType;
}

// ------------------------------ Netpbm image formats ----------------------------------
// Generic Mime type name

std::string CoreIOMimeTypes::PNM_MIMETYPE_NAME()
{
  // Have to pick one that becomes the default. PPM is the least restrictive.
  static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".ppm";
  return name;
}

// Specific Mime type names
std::string CoreIOMimeTypes::PBM_MIMETYPE_NAME()
{
  static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".pbm";
  return name;
}

std::string CoreIOMimeTypes::PGM_MIMETYPE_NAME()
{
  static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".pgm";
  return name;
}

std::string CoreIOMimeTypes::PPM_MIMETYPE_NAME()
{
  static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".ppm";
  return name;
}

// ------------------------------ Netpbm image formats ----------------------------------
// Generic Mime type descriptions
std::string CoreIOMimeTypes::PNM_MIMETYPE_DESCRIPTION()
{
  static std::string description = "The portable pixmap format (PPM), the portable graymap format (PGM) and the portable bitmap format (PBM) are image file formats used and defined by the Netpbm project.";
  return description;
}

// Specific Mime type descriptions
std::string CoreIOMimeTypes::PBM_MIMETYPE_DESCRIPTION()
{
  static std::string description = "Image in Portable BitMap format";
  return description;
}

std::string CoreIOMimeTypes::PGM_MIMETYPE_DESCRIPTION()
{
  static std::string description = "Image in Portable GreyMap format";
  return description;
}

std::string CoreIOMimeTypes::PPM_MIMETYPE_DESCRIPTION()
{
  static std::string description = "Image in Portable PixelMap format";
  return description;
}


} // end namespace
