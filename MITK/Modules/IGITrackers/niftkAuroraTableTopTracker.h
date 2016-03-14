/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkAuroraTableTopTracker_h
#define niftkAuroraTableTopTracker_h

#include <niftkIGITrackersExports.h>
#include "niftkNDITracker.h"

namespace niftk
{

/**
 * \class AuroraTableTopTracker
 * \brief RAII object to connect to Aurora Table Top tracker.
 */
class NIFTKIGITRACKERS_EXPORT AuroraTableTopTracker : public niftk::NDITracker
{
public:

  mitkClassMacroItkParent(AuroraTableTopTracker, niftk::NDITracker);
  mitkNewMacro3Param(AuroraTableTopTracker, mitk::DataStorage::Pointer, std::string, std::string);

protected:

  AuroraTableTopTracker(mitk::DataStorage::Pointer dataStorage,
                 std::string portName,
                 std::string toolConfigFileName); // Purposefully hidden.

  virtual ~AuroraTableTopTracker(); // Purposefully hidden.

  AuroraTableTopTracker(const AuroraTableTopTracker&); // Purposefully not implemented.
  AuroraTableTopTracker& operator=(const AuroraTableTopTracker&); // Purposefully not implemented.

private:

}; // end class

} // end namespace

#endif
