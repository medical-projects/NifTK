/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkFindAndTriangulateCrossHair.h"
#include <mitkCameraCalibrationFacade.h>
#include <mitkOpenCVMaths.h>
#include <mitkOpenCVImageProcessing.h>
#include <mitkOpenCVFileIOUtils.h>
#include <mitkTimeStampsContainer.h>
#include <cv.h>
#include <highgui.h>
#include <niftkFileHelper.h>

namespace mitk {

//-----------------------------------------------------------------------------
FindAndTriangulateCrossHair::FindAndTriangulateCrossHair()
: m_Visualise(false)
, m_SaveVideo(false)
, m_VideoIn("")
, m_VideoOut("")
, m_Directory("")
, m_TrackerIndex(0)
, m_TrackerMatcher(NULL)
, m_InitOK(false)
, m_TriangulateOK(false)
, m_LeftIntrinsicMatrix (new cv::Mat(3,3,CV_64FC1))
, m_LeftDistortionVector (new cv::Mat(1,4,CV_64FC1))
, m_RightIntrinsicMatrix (new cv::Mat(3,3,CV_64FC1))
, m_RightDistortionVector (new cv::Mat(1,4,CV_64FC1))
, m_RightToLeftRotationMatrix (new cv::Mat(3,3,CV_64FC1))
, m_RightToLeftTranslationVector (new cv::Mat(3,1,CV_64FC1))
, m_VideoWidth (0.0)
, m_VideoHeight (0.0)
, m_DefaultVideoWidth (1920)
, m_DefaultVideoHeight (540)
, m_FramesToProcess (-1)
, m_LeftCameraToTracker (new cv::Mat(4,4,CV_64FC1))
, m_Capture(NULL)
, m_FlipVideo(false)
, m_HaltOnVideoReadFail(true)
, m_Writer(NULL)
, m_BlurKernel (cv::Size (3,3))
, m_CannyLowThreshold(20)
, m_CannyHighThreshold(70)
, m_CannyKernel(3)
, m_HoughRho (1.0)
, m_HoughTheta(CV_PI/(180))
, m_HoughThreshold(50)
, m_HoughLineLength(130)
, m_HoughLineGap(20)
{
}


//-----------------------------------------------------------------------------
FindAndTriangulateCrossHair::~FindAndTriangulateCrossHair()
{
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::Initialise(std::string directory, 
    std::string calibrationParameterDirectory)
{
  m_InitOK = false;
  m_Directory = directory;

  try
  {
    mitk::LoadStereoCameraParametersFromDirectory
      ( calibrationParameterDirectory,
      m_LeftIntrinsicMatrix, m_LeftDistortionVector, m_RightIntrinsicMatrix,
      m_RightDistortionVector, m_RightToLeftRotationMatrix,
      m_RightToLeftTranslationVector, m_LeftCameraToTracker);
  }
  catch ( int e )
  {
    MITK_WARN << "Failed to load camera parameters";
    m_InitOK = false;
    return;
  }
  if ( m_TrackerMatcher.IsNull() ) 
  {
    m_TrackerMatcher = mitk::VideoTrackerMatching::New();
  }
  if ( ! m_TrackerMatcher->IsReady() )
  {
    m_TrackerMatcher->Initialise(m_Directory);
  }
  if ( ! m_TrackerMatcher->IsReady() )
  {
    MITK_WARN << "Failed to initialise tracker matcher";
    m_InitOK = false;
    return;
  }
  
  m_TrackerMatcher->SetCameraToTracker(*m_LeftCameraToTracker);

  if ( m_Capture == NULL ) 
  {
    m_VideoIn = niftk::FindVideoFile(m_Directory,  niftk::Basename (niftk::Basename ( m_TrackerMatcher->GetFrameMap())));
    if ( m_VideoIn == ""  ) 
    {
      m_InitOK = false;
      return;
    }
   
    try
    {
      m_Capture = mitk::InitialiseVideoCapture(m_VideoIn, (! m_HaltOnVideoReadFail) ); 
    }
    catch ( std::exception& e)
    {
      MITK_ERROR << "Caught exception " << e.what();
      exit(1);
    }
    //the following don't seem to work unless opencv is built with ffmpeg
    m_VideoWidth = static_cast<double>(m_Capture->get(CV_CAP_PROP_FRAME_WIDTH));
    m_VideoHeight = static_cast<double>(m_Capture->get(CV_CAP_PROP_FRAME_HEIGHT));
   
    if ( m_VideoWidth == 0.0 || m_VideoHeight == 0.0 )
    {
      MITK_WARN << "Failed to open " << m_VideoIn.c_str() << " correctly and m_HaltOnVideoReadFail false, attempting to continue with m_VideoWidth = " << m_DefaultVideoWidth << " and m_VideoHeight =  " << m_DefaultVideoHeight;
      m_VideoWidth = m_DefaultVideoWidth;
      m_VideoHeight = m_DefaultVideoHeight;
    }
    MITK_INFO << "Opened " << m_VideoIn << " ( " << m_VideoWidth << " x " << m_VideoHeight << " )";
  }

  m_InitOK = true;
  return;

}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::SetVisualise ( bool visualise )
{
  m_Visualise = visualise;
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::SetSaveVideo ( bool savevideo )
{
  if ( m_InitOK ) 
  {
    MITK_WARN << "Changing save video  state after initialisation, will need to re-initialise";
  }

  m_SaveVideo = savevideo;
  m_InitOK = false;
  return;
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::Triangulate()
{
  if ( ! m_InitOK )
  {
    MITK_WARN << "Called triangulate before initialise.";
    return;
  }
    
  m_TriangulateOK = false;
  m_WorldPoints.clear();
  m_PointsInLeftLensCS.clear();
  m_ScreenPoints.clear();

  if ( m_Visualise ) 
  {
    cvNamedWindow ("Left Channel", CV_WINDOW_AUTOSIZE);
    cvNamedWindow ("Right Channel", CV_WINDOW_AUTOSIZE);
  }
  int framenumber = 0 ;
  int key = 0;

  cv::Mat leftFrame;
  cv::Mat rightFrame;
  m_ScreenPoints.clear();
  int terminator = m_TrackerMatcher->GetNumberOfFrames();
  TimeStampsContainer::TimeStamp timeStamp;

  if ( m_FramesToProcess >= 0 ) 
  {
    terminator = m_FramesToProcess;
  }
  while ( framenumber < terminator && key != 'q')
  {
    cv::Mat videoImage;
    bool leftSuccess = m_Capture->read(videoImage);
    if ( m_FlipVideo )
    {
      int flipMode = 0 ; // flip around the x axis
      cv::flip(videoImage,leftFrame,flipMode);
    }
    else
    {
      leftFrame = videoImage.clone();
    }

    bool rightSuccess = m_Capture->read(videoImage);
    if ( m_FlipVideo )
    {
      int flipMode = 0 ; // flip around the x axis
      cv::flip(videoImage,rightFrame,flipMode);
    }
    else
    {
      rightFrame = videoImage.clone();
    }

    m_TrackerMatcher->GetVideoFrame(framenumber, &timeStamp);

    mitk::ProjectedPointPair screenPoints;
    screenPoints.m_Left = cv::Point2d(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    screenPoints.m_Right = cv::Point2d(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    
    if ( ( ! leftSuccess ) || ( ! rightSuccess ) )
    {
      m_ScreenPoints.push_back(screenPoints);
      m_ScreenPoints.push_back(screenPoints);
      MITK_WARN << "Failed to read video at frame " << framenumber;
    }
    else
    {
      cv::vector <cv::Vec4i> linesleft;
      cv::vector <cv::Vec4i> linesright;

      screenPoints.m_Left = mitk::FindCrosshairCentre ( leftFrame,
        m_CannyLowThreshold, m_CannyHighThreshold, m_CannyKernel, 
        m_HoughRho, m_HoughTheta, m_HoughThreshold , m_HoughLineLength, m_HoughLineGap, 
        linesleft);

      screenPoints.m_Right = mitk::FindCrosshairCentre ( rightFrame,
        m_CannyLowThreshold, m_CannyHighThreshold, m_CannyKernel, 
        m_HoughRho, m_HoughTheta, m_HoughThreshold , m_HoughLineLength, m_HoughLineGap,
        linesright);

      screenPoints.SetTimeStamp(timeStamp);
      //push_back twice because we're jumping two frames
      m_ScreenPoints.push_back(screenPoints);
      m_ScreenPoints.push_back(screenPoints);
      if ( m_Visualise ) 
      {
       /* for ( unsigned int i = 0 ; i < linesleft.size() ; i ++ )
        {
          cv::line(leftFrame,cvPoint(linesleft[i][0],linesleft[i][1]),
            cvPoint(linesleft[i][2],linesleft[i][3]),cvScalar(255,0,0));
        }
        for ( unsigned int i = 0 ; i < linesright.size() ; i ++ )
        {
          cv::line(rightFrame,cvPoint(linesright[i][0],linesright[i][1]),
            cvPoint(linesright[i][2],linesright[i][3]),cvScalar(255,0,0));
        }*/

        cv::circle(leftFrame , screenPoints.m_Left,10, cvScalar(0,0,255),2,8,0);
        cv::circle(rightFrame , screenPoints.m_Right,10, cvScalar(0,255,0),2,8,0);

        IplImage leftIpl(leftFrame);
        IplImage rightIpl(rightFrame);
        IplImage *smallleft = cvCreateImage (cvSize(960, 270), 8,3);
        cvResize (&leftIpl, smallleft,CV_INTER_LINEAR);
        IplImage *smallright = cvCreateImage (cvSize(960, 270), 8,3);
        cvResize (&rightIpl, smallright,CV_INTER_LINEAR);
        cvShowImage("Left Channel" , smallleft);
        cvShowImage("Right Channel" , smallright);
        key = cvWaitKey (20);
        if ( key == 's' )
        {
          m_Visualise = false;
        }
      }
    }
    framenumber ++;
    framenumber ++;
  }
  if ( m_ScreenPoints.size() !=  (unsigned int)terminator )
  {
    MITK_ERROR << "Got the wrong number of screen point pairs " << m_ScreenPoints.size() 
      << " != " << m_TrackerMatcher->GetNumberOfFrames();
    m_TriangulateOK = false;
  }
  else
  {
    m_TriangulateOK = true;
  }

  TriangulatePoints();
  TransformPointsToWorld();
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::TriangulatePoints()
{
  if ( ! m_TriangulateOK ) 
  {
    MITK_WARN << "Need to call triangulate before triangulate points";
    return;
  }
  
  bool cropUndistortedPoints = true;
  double cropValue = std::numeric_limits<double>::quiet_NaN();
  m_PointsInLeftLensCS = mitk::Triangulate (
       m_ScreenPoints,
       *m_LeftIntrinsicMatrix, 
       *m_LeftDistortionVector, 
       *m_RightIntrinsicMatrix, 
       *m_RightDistortionVector,
       *m_RightToLeftRotationMatrix,
       *m_RightToLeftTranslationVector,
       cropUndistortedPoints,
       0.0, m_VideoWidth, 0.0, m_VideoHeight, cropValue );
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::TransformPointsToWorld()
{
  if ( m_PointsInLeftLensCS.size() == 0  ) 
  {
    MITK_WARN << "Need to triangulate points before transforming to world";
    return;
  }

  for ( unsigned int i = 0 ; i < m_PointsInLeftLensCS.size() ; i ++ )
  {
    int framenumber = i;
    m_WorldPoints.push_back( m_TrackerMatcher->GetCameraTrackingMatrix(framenumber, NULL , m_TrackerIndex) * m_PointsInLeftLensCS[i]);
  }
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::SetFlipMatrices(bool state)
{
  if ( m_TrackerMatcher.IsNull() ) 
  {
    MITK_ERROR << "Tried to set flip matrices before initialisation";
    return;
  }

  m_TrackerMatcher->SetFlipMatrices(state);
}


//-----------------------------------------------------------------------------
void FindAndTriangulateCrossHair::SetVideoLagMilliseconds ( unsigned long long videoLag, bool videoLeadsTracking)
{
  if ( m_TrackerMatcher.IsNull()  || (! m_TrackerMatcher->IsReady()) )
  {
    MITK_ERROR << "Need to initialise before setting video lag";
    return;
  }

  m_TrackerMatcher->SetVideoLagMilliseconds (videoLag, videoLeadsTracking);
}

} // end namespace
