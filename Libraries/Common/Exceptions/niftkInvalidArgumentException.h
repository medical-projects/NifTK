/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkInvalidArgumentException_h
#define niftkInvalidArgumentException_h

#include "niftkExceptionObject.h"

namespace niftk
{

/**
* \brief Exceptions for signalling invalid input.
*/
class InvalidArgumentException : public ExceptionObject
{
public:
  InvalidArgumentException(const std::string &what) : ExceptionObject(what) {}
};

} // end namespace

#endif
