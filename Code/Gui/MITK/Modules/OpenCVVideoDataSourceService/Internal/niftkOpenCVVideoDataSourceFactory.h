/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkOpenCVVideoDataSourceFactory_h
#define niftkOpenCVVideoDataSourceFactory_h

#include <niftkIGIDataSourceFactoryServiceI.h>

namespace niftk
{

/**
* \class OpenCVVideoDataSourceFactory
* \brief Factory class to create OpenCVVideoDataSources.
 */
class OpenCVVideoDataSourceFactory : public IGIDataSourceFactoryServiceI
{

public:

  OpenCVVideoDataSourceFactory();
  virtual ~OpenCVVideoDataSourceFactory();

  virtual IGIDataSourceI::Pointer Create(mitk::DataStorage::Pointer dataStorage) override;

};

} // end namespace

#endif
