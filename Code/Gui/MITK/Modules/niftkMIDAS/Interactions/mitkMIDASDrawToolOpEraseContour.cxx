/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkMIDASDrawToolOpEraseContour.h"

namespace mitk {

MIDASDrawToolOpEraseContour::MIDASDrawToolOpEraseContour(
  mitk::OperationType type,
  mitk::ContourModelSet* contourModelSet,
  const int& workingNodeNumber
  )
: mitk::Operation(type)
, m_ContourModelSet(contourModelSet)
, m_WorkingNodeNumber(workingNodeNumber)
{

}

} // end namespace