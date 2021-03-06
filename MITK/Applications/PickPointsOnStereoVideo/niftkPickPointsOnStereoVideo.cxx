/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include <cstdlib>
#include <limits>
#include <fstream>

#include <mitkPickPointsOnStereoVideo.h>
#include <niftkPickPointsOnStereoVideoCLP.h>

int main(int argc, char** argv)
{
  PARSE_ARGS;
  int returnStatus = EXIT_FAILURE;

  if ( trackingInputDirectory.length() == 0 )
  {
    std::cout << trackingInputDirectory.length() << std::endl;
    commandLine.getOutput()->usage(commandLine);
    return returnStatus;
  }


  try
  {
    mitk::PickPointsOnStereoVideo::Pointer projector = mitk::PickPointsOnStereoVideo::New();
    projector->SetAllowableTimingError(maxTimingError * 1e6);
    projector->SetFrequency(frequency*2);
    projector->SetOrderedPoints( ! unOrderedPoints);
    projector->SetAskOverWrite(queryOverWrite);
    projector->SetWriteAnnotatedImages(saveAnnotatedImages);
    projector->SetHalfImageWidth(halfImageWidth);
    projector->SetFlipVideo(flipVideo);
    
    projector->Initialise(trackingInputDirectory);
    mitk::VideoTrackerMatching::Pointer matcher = mitk::VideoTrackerMatching::New();
    matcher->Initialise(trackingInputDirectory);
    if ( videoLag != 0 ) 
    {
      if ( videoLag < 0 )
      {
        matcher->SetVideoLagMilliseconds(videoLag,true);
      }
      else 
      {
        matcher->SetVideoLagMilliseconds(videoLag,false);
      }
    }

    if ( ! projector->GetInitOK() ) 
    {
      MITK_ERROR << "Projector failed to initialise, halting.";
      return -1;
    }
    bool flipTracking = false; 
    matcher->SetFlipMatrices(flipTracking);
    matcher->SetWriteTimingErrors(WriteTimingErrors);
    projector->SetTrackerIndex(trackerIndex);
    projector->SetReferenceIndex(referenceIndex);

    projector->Project(matcher);

    returnStatus = EXIT_SUCCESS;
  }
  catch (mitk::Exception& e)
  {
    MITK_ERROR << "Caught mitk::Exception: " << e.GetDescription() << ", from:" << e.GetFile() << "::" << e.GetLine() << std::endl;
    returnStatus = EXIT_FAILURE + 100;
  }
  catch (std::exception& e)
  {
    MITK_ERROR << "Caught std::exception: " << e.what() << std::endl;
    returnStatus = EXIT_FAILURE + 101;
  }
  catch (...)
  {
    MITK_ERROR << "Caught unknown exception:" << std::endl;
    returnStatus = EXIT_FAILURE + 102;
  }
  return returnStatus;
}
