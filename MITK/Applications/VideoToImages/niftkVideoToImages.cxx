/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include <cstdlib>
#include <fstream>
#include <limits>
#include <set>

#include <mitkExceptionMacro.h>
#include <mitkOpenCVFileIOUtils.h>
#include <mitkOpenCVPointTypes.h>

#include <niftkFileHelper.h>
#include <niftkFileIOUtils.h>
#include <niftkVideoToImagesCLP.h>

int main(int argc, char** argv)
{
  PARSE_ARGS;
  int returnStatus = EXIT_FAILURE;

  try
  {
    // Early exit if main input directory not specified.
    if (videoInputDirectory.size() == 0)
    {
      commandLine.getOutput()->usage(commandLine);
      return returnStatus;
    }

    // Error if input directory does not exist.
    if (!niftk::DirectoryExists(videoInputDirectory))
    {
      mitkThrow() << "Directory:" << videoInputDirectory << ", doesn't exist!";
    }

    // If user specifies a list of timeStamps, we should load that.
    std::set<unsigned long long> setOfTimeStamps;
    niftk::LoadTimeStampData(timeStamps, setOfTimeStamps);

    // Now read video and frameMap.
    std::vector <std::string> videoFiles = niftk::FindVideoData(videoInputDirectory);
    std::vector <std::string> frameMapFiles = mitk::FindVideoFrameMapFiles(videoInputDirectory);

    if ( videoFiles.size() > 1 )
    {
      MITK_WARN << "Found multiple video files, will only use " << videoFiles[0];
    }
    if ( frameMapFiles.size() > 1 )
    {
      MITK_WARN << "Found multiple framemap files, will only use " << frameMapFiles[0];
    }

    if ( videoFiles.size() == 0 || frameMapFiles.size() == 0 ) 
    {
      MITK_ERROR << "Failed to find video and/or frameMapFiles in directory " << videoInputDirectory;
      exit(1);
    }

    cv::VideoCapture* capture;
    
    try 
    {
      capture = mitk::InitialiseVideoCapture(videoFiles[0], ignoreVideoReadFail);
    }
    catch (std::exception& e)
    {
      MITK_ERROR << "Caught std::exception:" << e.what();
      returnStatus = -1;
      return returnStatus;
    }
    catch (...)
    {
      MITK_ERROR << "Caught unknown exception:";
      returnStatus = -2;
      return returnStatus;
    }

    std::ifstream* fin = new std::ifstream(frameMapFiles[0].c_str());

    if ( (! capture) || (!fin) )
    {
      MITK_ERROR << "Failed to open video and/or frameMapFile";
      exit(1);
    }
    
    unsigned long framecount = 0 ;
    if ( framesToUse < 0 || setOfTimeStamps.size() > 0)
    {
      MITK_INFO << "Scanning whole file" << std::endl;
      framesToUse = std::numeric_limits<double>::infinity();
    }

    while(framecount < framesToUse )
    {
      mitk::VideoFrame frame;
      try
      {
        frame = mitk::VideoFrame(capture, fin);

      }
      catch (std::exception& e)
      {
        MITK_ERROR << "Caught exception:" << e.what();
        break;
      }

      if (   setOfTimeStamps.size() == 0 // user did not specify timestamps
          || setOfTimeStamps.find(frame.GetTimeStamp()) != setOfTimeStamps.end() // user specified timestamps, and it matches.
          )
      {
        MITK_INFO << "Writing frame " << framecount << ", timestamp=" << frame.GetTimeStamp() << std::endl;
        frame.WriteToFile(outputPrefix, outputExtension);
      }
      framecount ++;
    }

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
