/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkMakeMaskImagesFromStereoVideo.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <cv.h>
//#include <opencv2/highgui/highgui.hpp>
#include <highgui.h>

#include <mitkCameraCalibrationFacade.h>
#include <mitkOpenCVFileIOUtils.h>
#include <mitkOpenCVMaths.h>
#include <mitkOpenCVPointTypes.h>

#include <niftkFileHelper.h>
#include <niftkFileIOUtils.h>

namespace mitk
{

//-----------------------------------------------------------------------------
MakeMaskImagesFromStereoVideo::MakeMaskImagesFromStereoVideo()
: 
m_VideoIn("")
, m_Directory("")
, m_TrackerIndex(0)
, m_ReferenceIndex(-1)
, m_InitOK(false)
, m_ProjectOK(false)
, m_VideoWidth(1920)
, m_VideoHeight(540)
, m_Capture(NULL)
, m_AllowableTimingError (20e6) // 20 milliseconds 
, m_AskOverWrite(false)
, m_HaltOnVideoReadFail(true)
, m_FlipVideo(false)
, m_StartFrame(12) //by default we select different frames than mitkPickPointsOnStereoVideo
, m_EndFrame(0)
, m_Frequency(150)
{
}

//-----------------------------------------------------------------------------
MakeMaskImagesFromStereoVideo::~MakeMaskImagesFromStereoVideo()
{
}

//-----------------------------------------------------------------------------
void MakeMaskImagesFromStereoVideo::Initialise(std::string directory)
{
  m_InitOK = false;
  m_Directory = directory;
  m_OutDirectory = m_Directory + niftk::GetFileSeparator() +  "VideoMasks";
  niftk::LoadTimeStampData(m_TimeStampsFile, m_TimeStampsSet);
  if (m_TimeStampsSet.size() > 0)
  {
    m_Frequency = 1;
  }
  m_InitOK = true;
  return;

}

//-----------------------------------------------------------------------------
void MakeMaskImagesFromStereoVideo::Project(mitk::VideoTrackerMatching::Pointer trackerMatcher)
{
  if ( ! m_InitOK )
  {
    MITK_WARN << "Called project before initialise.";
    return;
  }
  
  if ( m_Capture == NULL ) 
  {
    m_VideoIn = niftk::FindVideoFile(m_Directory, niftk::Basename (niftk::Basename ( trackerMatcher->GetFrameMap() )));
    if ( m_VideoIn == "" ) 
    {
      m_InitOK = false;
      return;
    }
    try
    {
      m_Capture = mitk::InitialiseVideoCapture(m_VideoIn, (! m_HaltOnVideoReadFail )); 
    }
    catch (std::exception& e)
    {
      MITK_ERROR << "Caught exception " << e.what();
      exit(1);
    }
    try
    {
      niftk::CreateDirAndParents ( m_OutDirectory );
    }
    catch (std::exception& e)
    {
      MITK_ERROR << "Caught exception " << e.what();
      exit(1);
    }
  }
   
  m_ProjectOK = false;

  int framenumber = 0 ;
  int key = 0;
  int annotationLineThickness = 1;
  cvNamedWindow ("Left Channel", CV_WINDOW_AUTOSIZE);
  cvNamedWindow ("Right Channel", CV_WINDOW_AUTOSIZE);
     
  cv::Mat blankMat = cvCreateMat(10,100,CV_32FC3);
  IplImage blankImage(blankMat);
  cvShowImage("Left Channel" , &blankImage);
  cvShowImage("Right Channel" , &blankImage);
  unsigned long long startTime;
  trackerMatcher->GetVideoFrame(0, &startTime);
  while ( framenumber < trackerMatcher->GetNumberOfFrames() && key != 'q')
  {
    if ( ( m_StartFrame < m_EndFrame ) && ( framenumber < m_StartFrame || framenumber > m_EndFrame ) )
    {
      cv::Mat videoImage;
      m_Capture->read(videoImage);
      MITK_INFO << "Skipping frame " << framenumber;
      framenumber ++;
    }
    else
    {
      long long timingError;
      cv::Mat WorldToLeftCamera = trackerMatcher->GetCameraTrackingMatrix(framenumber, &timingError, m_TrackerIndex, NULL, m_ReferenceIndex).inv();

      cv::Mat tempMat;

      cv::Mat leftVideoImage;
      cv::Mat rightVideoImage;
      m_Capture->read(tempMat);
      if ( m_FlipVideo )
      {
        int flipMode = 0 ; // flip around the x axis
        cv::flip(tempMat,leftVideoImage,flipMode);
      }
      else
      {
        leftVideoImage = tempMat.clone();
      }

      m_Capture->read(tempMat);
      if ( m_FlipVideo )
      {
        int flipMode = 0 ; // flip around the x axis
        cv::flip(tempMat,rightVideoImage,flipMode);
      }
      else
      {
        rightVideoImage = tempMat.clone();
      }

      if ( std::abs(timingError) <  m_AllowableTimingError )
      {
        key = cvWaitKey (20);

        if ( (framenumber-m_StartFrame) %m_Frequency == 0 ) 
        {
          unsigned long long timeStamp;
          trackerMatcher->GetVideoFrame(framenumber, &timeStamp);

          if (m_TimeStampsSet.size() == 0
              || (m_TimeStampsSet.size() != 0 && m_TimeStampsSet.find(timeStamp) != m_TimeStampsSet.end())
              )
          {
            MITK_INFO << "Picking contours on frame pair " << framenumber << ", " << framenumber+1 << " [ " << (timeStamp - startTime)/1e9 << " s ], c to make mask image, n for next frame, q to quit";

            std::string leftOutName = m_OutDirectory + niftk::GetFileSeparator() + boost::lexical_cast<std::string>(timeStamp);
            trackerMatcher->GetVideoFrame(framenumber+1, &timeStamp);
            std::string rightOutName = m_OutDirectory + niftk::GetFileSeparator() + boost::lexical_cast<std::string>(timeStamp);
            bool overWriteLeft = true;
            bool overWriteRight = true;
            key = 0;
            if ( boost::filesystem::exists (leftOutName + "_leftContour.xml") )
            {
              if ( m_AskOverWrite )
              {
                MITK_INFO << leftOutName + "_leftContour.xml" << " exists, overwrite (y/n)";
                key = 0;
                while ( ! ( key == 'n' || key == 'y' ) )
                {
                  key = cvWaitKey(20);
                }
                if ( key == 'n' )
                {
                  overWriteLeft = false;
                }
                else
                {
                  overWriteLeft = true;
                }
              }
              else
              {
                MITK_INFO << leftOutName + "_leftContour.xml" << " exists, skipping.";
                overWriteLeft = false;
              }
            }

            key = 0;
            if ( boost::filesystem::exists (rightOutName + "_rightContour.xml") )
            {
              if ( m_AskOverWrite )
              {
                MITK_INFO << rightOutName  + "_rightContour.xml" << " exists, overwrite (y/n)";
                while ( ! ( key == 'n' || key == 'y' ) )
                {
                  key = cv::waitKey(20);
                }
                if ( key == 'n' )
                {
                  overWriteRight = false;
                }
                else
                {
                  overWriteRight = true;
                }
              }
              else
              {
                MITK_INFO << rightOutName + "_rightContour.xml" << " exists, skipping.";
                overWriteRight = false;
              }
            }

            PickedPointList::Pointer leftPickedPoints = PickedPointList::New();
            PickedPointList::Pointer rightPickedPoints = PickedPointList::New();
            leftPickedPoints->SetFrameNumber (framenumber);
            leftPickedPoints->SetChannel ("left");
            leftPickedPoints->SetTimeStamp(timeStamp);
            leftPickedPoints->SetInLineMode (true);
            leftPickedPoints->SetInOrderedMode (false);
            rightPickedPoints->SetFrameNumber (framenumber + 1);
            rightPickedPoints->SetChannel ("right");
            rightPickedPoints->SetTimeStamp(timeStamp);
            rightPickedPoints->SetInLineMode (true);
            rightPickedPoints->SetInOrderedMode (false);

            cv::Mat leftAnnotatedVideoImage = leftVideoImage.clone();
            cv::Mat rightAnnotatedVideoImage = rightVideoImage.clone();
            cv::Mat leftMaskImage;
            cv::Mat rightMaskImage;
            bool showMasks = false;
            key = 0;
            if ( overWriteLeft  ||  overWriteRight  )
            {
              while ( key != 'n' && key != 'q' )
              {
                key = cv::waitKey(20);
                if ( key == 'c' )
                {
                  if ( showMasks )
                  {
                    MITK_INFO << "Exiting contour mode";
                  }
                  else
                  {
                    MITK_INFO << "Attempting to make contour images";
                    leftMaskImage = leftPickedPoints->CreateMaskImage ( leftAnnotatedVideoImage );
                    rightMaskImage = rightPickedPoints->CreateMaskImage ( rightAnnotatedVideoImage );
                    cv::imwrite(leftOutName + "_leftMask.png" ,leftMaskImage);
                    cv::imwrite(rightOutName + "_rightMask.png" ,rightMaskImage);
                    cv::imwrite(leftOutName + "_left.png" ,leftVideoImage);
                    cv::imwrite(rightOutName + "_right.png" ,rightVideoImage);
                  }
                  showMasks = ! showMasks;
                }
                if ( overWriteLeft )
                {
                  cvSetMouseCallback("Left Channel",PointPickingCallBackFunc, leftPickedPoints);
                  if ( leftPickedPoints->GetIsModified() )
                  {
                    leftAnnotatedVideoImage = leftVideoImage.clone();
                    leftPickedPoints->AnnotateImage(leftAnnotatedVideoImage, annotationLineThickness);

                    std::ofstream leftPointOut ((leftOutName+ "_leftContour.xml").c_str());
                    leftPickedPoints->PutOut( leftPointOut );
                    leftPointOut.close();
                  }
                  if ( showMasks )
                  {
                    IplImage image(leftMaskImage);
                    cvShowImage("Left Channel" , &image);
                  }
                  else
                  {
                    IplImage image(leftAnnotatedVideoImage);
                    cvShowImage("Left Channel" , &image);
                  }
                }

                if ( overWriteRight )
                {
                  cvSetMouseCallback("Right Channel",PointPickingCallBackFunc, rightPickedPoints);
                  if ( rightPickedPoints->GetIsModified() )
                  {
                    rightAnnotatedVideoImage = rightVideoImage.clone();
                    rightPickedPoints->AnnotateImage(rightAnnotatedVideoImage, annotationLineThickness);

                    std::ofstream rightPointOut ((rightOutName + "_rightContour.xml").c_str());
                    rightPickedPoints->PutOut (rightPointOut);
                    rightPointOut.close();
                  }
                  if ( showMasks )
                  {
                    IplImage rimage(rightMaskImage);
                    cvShowImage("Right Channel" , &rimage);
                  }
                  else
                  {
                    IplImage rimage(rightAnnotatedVideoImage);
                    cvShowImage("Right Channel" , &rimage);
                  }
                }
              }
            }
            cvShowImage("Left Channel" , &blankImage);
            cvShowImage("Right Channel" , &blankImage);

          } // end if timestamp was specified, and it matches.

        } // end if (sampling every 150 frames or so).
      }
      else
      {
        MITK_INFO << "Skipping frame " << framenumber << " high timing error " << timingError;
      }
      framenumber += 2;
    }
  }
  m_ProjectOK = true;
}

} // end namespace
