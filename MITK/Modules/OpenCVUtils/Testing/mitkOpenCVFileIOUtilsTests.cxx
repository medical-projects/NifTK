/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include <niftkFileHelper.h>
#include <mitkTestingMacros.h>
#include <mitkLogMacros.h>
#include <mitkOpenCVFileIOUtils.h>
#include <mitkOpenCVMaths.h>
#include <mitkIOUtil.h>
#include <cmath>

/**
 * \file Tests for some of the functions in openCVFileIOUtils.
 */

void LoadTimeStampedPointsTest(std::string dir)
{
  std::string pointdir = dir + "pins";
  std::string matrixdir = dir + "Aurora_2/1";

  std::vector < std::pair<unsigned long long, cv::Point3d> > timeStampedPoints = mitk::LoadTimeStampedPoints(pointdir);

  MITK_TEST_CONDITION ( timeStampedPoints.size() == 102 , "Testing 102 points were loaded. " << timeStampedPoints.size() );
  MITK_TEST_CONDITION ( timeStampedPoints[0].first == 1429793163701888600 , "Testing first time stamp " <<  timeStampedPoints[0].first);
  MITK_TEST_CONDITION ( timeStampedPoints[101].first == 1429793858654637600 , "Testing last time stamp " <<  timeStampedPoints[101].first);

  MITK_TEST_CONDITION ( mitk::NearlyEqual (timeStampedPoints[0].second,cv::Point3d (317,191.0,0.0),1e-6), "Testing first point value " <<  timeStampedPoints[0].second);
  MITK_TEST_CONDITION ( mitk::NearlyEqual (timeStampedPoints[101].second, cv::Point3d (345.0, 162.0, 0.0),1e-6), "Testing last time stamp " <<  timeStampedPoints[87].second);

}

void TestLoadPickedObject ( char * filename )
{
  MITK_INFO << "Attemting to open " << filename;
  std::vector <mitk::PickedObject> p1;
  double yScale = 1.0;
  std::ifstream stream;
  stream.open(filename);
  if ( stream )
  {
    LoadPickedObjects ( p1, stream, yScale );
  }
  else
  {
    MITK_ERROR << "Failed to open " << filename;
  }
  MITK_TEST_CONDITION ( p1.size() == 13 , "Testing that 13 picked objects were read : " << p1.size());
  MITK_TEST_CONDITION ( p1[3].m_TimeStamp == 1421406496123439600, "Testing point 4 time stamp is 1421406496123439600 : " << p1[4].m_TimeStamp );
  MITK_TEST_CONDITION ( p1[1].m_Id == 2, "Testing point 1 ID is 2 : " << p1[2].m_Id );
  MITK_TEST_CONDITION ( p1[8].m_FrameNumber == 0, "Testing point 8 frame number  is 0 : " << p1[9].m_FrameNumber );
  MITK_TEST_CONDITION ( p1[9].m_Channel ==  "left", "Testing point 9 channel  is left : " << p1[10].m_Channel );
  MITK_TEST_CONDITION ( p1[0].m_IsLine , "Testing point 0 is line " << p1[0].m_IsLine );
  MITK_TEST_CONDITION ( ! p1[4].m_IsLine, "Testing point 4 is not a line " << p1[4].m_IsLine );
  MITK_TEST_CONDITION ( p1[11].m_Points.size() == 3 , "Testing there are 3 points in object 11" << p1[11].m_Points.size());
  MITK_TEST_CONDITION ( mitk::NearlyEqual(p1[12].m_Points[1], cv::Point3d ( 262, 98, 0), 1e-8 ) , "Testing value of point 2 in object 13" << p1[12].m_Points[1]);

}

void TestLoadPickedPointListFromDirectoryOfMPSFiles ( char * directory )
{
  mitk::PickedPointList::Pointer ppl = mitk::LoadPickedPointListFromDirectoryOfMPSFiles ( directory  );

  MITK_TEST_CONDITION ( ppl->GetListSize() == 9 , "Testing that there are 9 picked objects in the list : " << ppl->GetListSize() );
  MITK_TEST_CONDITION ( ppl->GetNumberOfPoints() == 5, "Testing that there are 5 picked points in the list : " << ppl->GetNumberOfPoints() );
  MITK_TEST_CONDITION ( ppl->GetNumberOfLines() == 4, "Testing that there are 4 picked lines in the list : " << ppl->GetNumberOfLines() );

  std::vector <mitk::PickedObject> pickedObjects = ppl->GetPickedObjects();

  bool point_0_found = false;
  bool point_3_found = false;
  bool point_5_found = false;
  bool line_4_found = false;
  for ( std::vector<mitk::PickedObject>::const_iterator it = pickedObjects.begin() ; it < pickedObjects.end() ; ++it )
  {
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 0 ) )
    {
      point_0_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 5 ) )
    {
      point_5_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 5 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == true ) && ( it->m_Id == 4 ) )
    {
      line_4_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-0.270583, -85.0786, 1510.05), 1e-6) ,
          "Testing Value of point 0 in line 4 = " << it->m_Points[0] );
    }
  }
  MITK_TEST_CONDITION ( point_0_found , "Testing that point 0 was found" );
  MITK_TEST_CONDITION ( ! point_3_found , "Testing that point 3 was not found" );
  MITK_TEST_CONDITION ( point_5_found , "Testing that point 5 was found" );
  MITK_TEST_CONDITION ( line_4_found , "Testing that line 4 was found" );

  //the base directory contains point lists in MITK's legacy format. Lets check that we get the same result with MITK's new format
  mitk::PickedPointList::Pointer ppl_v2 = mitk::LoadPickedPointListFromDirectoryOfMPSFiles ( directory + niftk::GetFileSeparator() + "v2" );
  MITK_TEST_CONDITION ( ppl_v2->GetListSize() == 6 , "Testing that there are 6 picked objects in the new formatt list : " << ppl_v2->GetListSize() );
  MITK_TEST_CONDITION ( ppl_v2->GetNumberOfPoints() == 5, "Testing that there are 5 picked points in the list : " << ppl_v2->GetNumberOfPoints() );
  MITK_TEST_CONDITION ( ppl_v2->GetNumberOfLines() == 1, "Testing that there are 1 picked lines in the list : " << ppl_v2->GetNumberOfLines() );

  pickedObjects = ppl_v2->GetPickedObjects();

  point_0_found = false;
  point_3_found = false;
  point_5_found = false;
  line_4_found = false;
  for ( std::vector<mitk::PickedObject>::const_iterator it = pickedObjects.begin() ; it < pickedObjects.end() ; ++it )
  {
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 0 ) )
    {
      point_0_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 5 ) )
    {
      point_5_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 5 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == true ) && ( it->m_Id == 4 ) )
    {
      line_4_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(-0.270583, -85.0786, 1510.05), 1e-6) ,
          "Testing Value of point 0 in line 4 = " << it->m_Points[0] );
    }
  }
  MITK_TEST_CONDITION ( point_0_found , "Testing that point 0 was found" );
  MITK_TEST_CONDITION ( ! point_3_found , "Testing that point 3 was not found" );
  MITK_TEST_CONDITION ( point_5_found , "Testing that point 5 was found" );
  MITK_TEST_CONDITION ( line_4_found , "Testing that line 4 was found" );

  //and what happens when we move it
  mitk::PickedPointList::Pointer ppl_v2_moved = mitk::LoadPickedPointListFromDirectoryOfMPSFiles ( directory + niftk::GetFileSeparator() + "v2_moved" );
  MITK_TEST_CONDITION ( ppl_v2_moved->GetListSize() == 6 , "Testing that there are 6 picked objects in the new formatt list : " << ppl_v2_moved->GetListSize() );
  MITK_TEST_CONDITION ( ppl_v2_moved->GetNumberOfPoints() == 5, "Testing that there are 5 picked points in the list : " << ppl_v2_moved->GetNumberOfPoints() );
  MITK_TEST_CONDITION ( ppl_v2_moved->GetNumberOfLines() == 1, "Testing that there are 1 picked lines in the list : " << ppl_v2_moved->GetNumberOfLines() );

  pickedObjects = ppl_v2_moved->GetPickedObjects();

  point_0_found = false;
  point_3_found = false;
  point_5_found = false;
  line_4_found = false;
  for ( std::vector<mitk::PickedObject>::const_iterator it = pickedObjects.begin() ; it < pickedObjects.end() ; ++it )
  {
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 0 ) )
    {
      point_0_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d( -88.6200, 691.7687, 1403.4441 ), 1e-3) ,
          "Testing Value of point 0 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == false ) && ( it->m_Id == 5 ) )
    {
      point_5_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(17.6141, 663.8431, 1448.5037), 1e-3) ,
          "Testing Value of point 5 = " << it->m_Points[0] );
    }
    if ( ( it->m_IsLine == true ) && ( it->m_Id == 4 ) )
    {
      line_4_found = true;
      MITK_TEST_CONDITION (mitk::NearlyEqual(it->m_Points[0], cv::Point3d(19.7294, 661.3448, 1450.2810), 1e-3) ,
          "Testing Value of point 0 in line 4 = " << it->m_Points[0] );
    }
  }
  MITK_TEST_CONDITION ( point_0_found , "Testing that point 0 was found" );
  MITK_TEST_CONDITION ( ! point_3_found , "Testing that point 3 was not found" );
  MITK_TEST_CONDITION ( point_5_found , "Testing that point 5 was found" );
  MITK_TEST_CONDITION ( line_4_found , "Testing that line 4 was found" );

}

void TestLoadMPSAndConvertToOpenCVVector ( char * directory )
{
  mitk::PointSet::Pointer pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() + "points.mps");

  std::vector < cv::Point3d > pointVector = mitk::PointSetToVector ( pointSet );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() , "Testing that point vector size == point set size " <<
      pointVector.size() << " == " << pointSet->GetSize() );
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[4], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 4 = " << pointVector[4] );

  //with new format
  pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() +
      "v2" + niftk::GetFileSeparator() + "points.mps");

  pointVector = mitk::PointSetToVector ( pointSet );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() , "Testing that point vector size == point set size " <<
      pointVector.size() << " == " << pointSet->GetSize() );
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[4], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 4 = " << pointVector[4] );

  //with new format after move
  pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() +
      "v2_moved" + niftk::GetFileSeparator() + "points.mps");

  pointVector = mitk::PointSetToVector ( pointSet );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() , "Testing that point vector size == point set size " <<
      pointVector.size() << " == " << pointSet->GetSize() );
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d( -88.6200, 691.7687, 1403.4441 ), 1e-3) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[4], cv::Point3d(17.6141, 663.8431, 1448.5037), 1e-3) ,
          "Testing Value of point 4 = " << pointVector[4] );

}

void TestLoadMPSAndConvertToOpenCVVectorWithIndexFilling ( char * directory )
{
  bool fillMissingIndices = true;
  //with index filling we should end up with one extra value in the vector, corresponding to the missing point 3
  mitk::PointSet::Pointer pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() + "points.mps");

  std::vector < cv::Point3d > pointVector = mitk::PointSetToVector ( pointSet, fillMissingIndices );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() + 1 , "Testing that point vector size == point set size + 1 " <<
      pointVector.size() << " == " << pointSet->GetSize() + 1 );
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[5], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 5 = " << pointVector[5] );

  //with new format
  pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() +
      "v2" + niftk::GetFileSeparator() + "points.mps");

  pointVector = mitk::PointSetToVector ( pointSet, fillMissingIndices );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() + 1 , "Testing that point vector size == point set size + 1 " <<
      pointVector.size() << " == " << pointSet->GetSize() + 1);
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d(-108.62, -35.3123, 1484.7), 1e-6) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[5], cv::Point3d(-2.38586, -82.0263, 1509.76), 1e-6) ,
          "Testing Value of point 5 = " << pointVector[5] );

  //with new format after move
  pointSet = mitk::IOUtil::LoadPointSet ( directory + niftk::GetFileSeparator() +
      "v2_moved" + niftk::GetFileSeparator() + "points.mps");

  pointVector = mitk::PointSetToVector ( pointSet, fillMissingIndices );

  MITK_TEST_CONDITION ( pointVector.size() == pointSet->GetSize() + 1, "Testing that point vector size == point set size + 1 " <<
      pointVector.size() << " == " << pointSet->GetSize() + 1 );
  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[0], cv::Point3d( -88.6200, 691.7687, 1403.4441 ), 1e-3) ,
          "Testing Value of point 0 = " << pointVector[0] );

  MITK_TEST_CONDITION (mitk::NearlyEqual(pointVector[5], cv::Point3d(17.6141, 663.8431, 1448.5037), 1e-3) ,
          "Testing Value of point 5 = " << pointVector[5] );

}

void TestInitialiseVideoCapture ( char * goodFile, char * dummyFile )
{
  try
  {
    bool ignoreErrors = false;
    cv::VideoCapture* capture = mitk::InitialiseVideoCapture ( goodFile , ignoreErrors );
    MITK_TEST_CONDITION ( ( capture != NULL ), "Testing initialised capture stream OK.");

    cv::Mat frame;
    if (  capture->read(frame) )
    {
      MITK_TEST_CONDITION ( ( frame.cols==1920 && frame.rows == 540 ), "Testing size of capture stream: " << frame.cols <<
        " by " << frame.rows );
    }
    else
    {
      MITK_TEST_CONDITION ( false , "Initialise video capture could not read frame." );
    }
  }
  catch (...)
  {
    MITK_TEST_CONDITION ( true , "Initialise video capture threw an exception, but that is expected as not all builds support .264 decoding." );
  }

  try
  {
    bool ignoreErrors = false;
    cv::VideoCapture* capture = mitk::InitialiseVideoCapture ( dummyFile , ignoreErrors );

    MITK_TEST_CONDITION ( false , "Initialise video capture did not throw an exception when reading dummy data." );
  }
  catch (...)
  {
    MITK_TEST_CONDITION ( true , "Initialise video capture threw an exception when reading dummy data." );
  }

}

void TestCreateVideoWriter ()
{
  //start by testing codecs
  int mpeg1codec = CV_FOURCC ( 'M','P','G','1');
  int mjpgcodec = CV_FOURCC ( 'M','J','P','G');
  int dvixcodec = CV_FOURCC ( 'D','V','I','X');
  int junkcodec = CV_FOURCC ( 'J','U','N','K');

  //mpeg1 is not supported on all platforms.
  bool mpeg1good = mitk::TestVideoWriterCodec ( mpeg1codec );
  if ( mpeg1good )
  {
    MITK_TEST_CONDITION ( true , "MPEG1 codec is usable." );
  }
  else
  {
    MITK_TEST_CONDITION ( true , "MPEG1 codec is not usable." );
  }

  //mjpg should always work, unless you're using a MAC
  bool mjpggood = mitk::TestVideoWriterCodec ( mjpgcodec );

#if defined(__APPLE__) || defined(__MACOSX)
  if ( mjpggood )
  {
    MITK_TEST_CONDITION ( true,  "MJPG codec works on this mac." );
  }
  else
  {
    MITK_TEST_CONDITION ( true, "MJPG codec doesn't work, but you're using an Apple Computer so that's OK." );
  }
#else
  MITK_TEST_CONDITION ( mjpggood , "Testing that MJPG codec works." );
#endif

  //dvix is not supported on all platforms.
  bool dvixgood = mitk::TestVideoWriterCodec ( dvixcodec );
  if ( dvixgood )
  {
    MITK_TEST_CONDITION ( true , "DVIX codec is usable." );
  }
  else
  {
    MITK_TEST_CONDITION ( true , "DVIX codec is not usable." );
  }

  //junk should never work
  bool junkgood = mitk::TestVideoWriterCodec ( junkcodec );
  MITK_TEST_CONDITION ( ! junkgood , "Testing that JUNK codec does not work." );

  //now test actual writer
  std::string outfile = niftk::CreateUniqueTempFileName ( "video", ".avi" );
  double frameRate = 25.0;
  cv::Size2i size = cv::Size2i ( 1920, 1080 );

  if ( mpeg1good )
  {
    cv::VideoWriter* writer  = mitk::CreateVideoWriter ( outfile , frameRate, size );
    MITK_TEST_CONDITION ( ( writer != NULL ), "Testing created video writer OK: " << outfile);
    cv::Mat frame = cv::Mat::eye (1080,1920,CV_8UC3);
    for ( unsigned int i = 0 ; i < 5 ; i ++ )
    {
      writer->write( frame );
    }
    MITK_TEST_CONDITION ( true , "Successfully pushed frame to writer." );
    writer->release();

    //I'd like to test the encoder better, but whole pipeline is very
    //unpredictable, the underlying encoder/decoder throws exceptions which aren't
    //easily caught, or fails to work and doesn't throw. So let's just
    //check it isn't empty.
    MITK_TEST_CONDITION ( ! niftk::FileIsEmpty (outfile),
        "Testing that " << outfile << " is not empty: filesize = " << niftk::FileSize(outfile) );
    MITK_TEST_CONDITION ( niftk::FileSize (outfile) >= 33232,
        "Testing that " << outfile << " filesize >= 33232 : actual size = " << niftk::FileSize(outfile) );
  }
  niftk::FileDelete ( outfile );

  //now try with a silly encoder, should create it, but fall back to mjpeg
  if ( mjpggood )
  {
    outfile = niftk::CreateUniqueTempFileName ( "video", ".avi" );
    cv::VideoWriter* writer  = mitk::CreateVideoWriter ( outfile , frameRate, size, junkcodec );
    MITK_TEST_CONDITION ( ( writer != NULL ), "Testing created video writer OK: " << outfile);
    cv::Mat frame = cv::Mat::eye (1080,1920,CV_8UC3);
    for ( unsigned int i = 0 ; i < 5 ; i ++ )
    {
      writer->write( frame );
    }
    MITK_TEST_CONDITION ( true , "Successfully pushed frame to writer." );
    writer->release();
    MITK_TEST_CONDITION ( ! niftk::FileIsEmpty (outfile),
      "Testing that " << outfile << " is not empty: filesize = " << niftk::FileSize(outfile));
    MITK_TEST_CONDITION ( niftk::FileSize (outfile) >= 171732,
      "Testing that " << outfile << " filesize >= 171732 : actual size = " << niftk::FileSize(outfile) );
    niftk::FileDelete ( outfile );
  }
}

void TestReadTrackerMatrix (char * filename , char * badfilename)
{
  cv::Mat test = cvCreateMat (4,4,CV_64FC1);

  bool returnStatus = mitk::ReadTrackerMatrix ( filename,test);

  MITK_TEST_CONDITION ( returnStatus , "Testing that ReadTrackerMatrix returns true for good file." );

  MITK_TEST_CONDITION ( test.at<double>( 0,0 ) == 0.6201384068 , "Testing that first element of matrix = 0.6201384068" );

  cv::Matx44d test44d;

  returnStatus = mitk::ReadTrackerMatrix ( filename, test44d );

  MITK_TEST_CONDITION ( returnStatus , "Testing that ReadTrackerMatrix (44d) returns true for good file." );

  MITK_TEST_CONDITION ( test44d( 0,0 ) == 0.6201384068 , "Testing that first element of matrix (44d) = 0.6201384068" );

  cv::Mat testBad1 = cvCreateMat (3,4,CV_64FC1);
  cv::Mat testBad2 = cvCreateMat (4,3,CV_64FC1);
  cv::Mat testBad3 = cvCreateMat (4,4,CV_32FC1);

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( filename, testBad1 );
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix did not throw an exception when matrix did not have 4 rows" );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( true, "ReadTrackerMatrix threw an exception when matrix did not have 4 rows: " << e.what() );
  }

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( filename, testBad2 );
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix did not throw an exception when matrix did not have 4 columns" );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( true, "ReadTrackerMatrix threw an exception when matrix did not have 4 columns: " << e.what() );
  }

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( filename, testBad3 );
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix did not throw an exception when matrix was not 64 bit float (64FC1)" );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( true, "ReadTrackerMatrix threw an exception when matrix was noy 64 bit float (64FC1) " << e.what() );
  }

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( "nonsence.nonsense" , test );
    MITK_TEST_CONDITION ( ! returnStatus , "Testing that ReadTrackerMatrix returns false for bad file." );
    MITK_TEST_CONDITION ( test.at<double>( 0,0 ) == 0.6201384068 , "Testing that first element of matrix was not altered by failed call to ReadTrackerMatrix." );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix threw an exception when file IO error: " << e.what() );
  }

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( "nonsence.nonsense" , test44d );
    MITK_TEST_CONDITION ( ! returnStatus , "Testing that ReadTrackerMatrix(44d) returns false for bad file." );
    MITK_TEST_CONDITION ( test44d( 0,0 ) == 0.6201384068 , "Testing that first element of matrix was not altered by failed call to ReadTrackerMatrix (44d)." );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix (44d) threw an exception when file IO error: " << e.what() );
  }

  try
  {
    returnStatus = mitk::ReadTrackerMatrix ( badfilename , test );
    MITK_TEST_CONDITION ( ! returnStatus , "Testing that ReadTrackerMatrix returns false for bad file format" );
    MITK_TEST_CONDITION ( test.at<double>( 0,0 ) == 0.6201384068 , "Testing that first element of matrix was not altered by failed call to ReadTrackerMatrix." );
  }
  catch ( std::exception e )
  {
    MITK_TEST_CONDITION ( false, "ReadTrackerMatrix threw an exception when file was badly formatted: " << e.what() );
  }

}

int mitkOpenCVFileIOUtilsTests(int argc, char * argv[])
{
  // always start with this!
  MITK_TEST_BEGIN("mitkOpenCVFileIOUtilsTests");

  LoadTimeStampedPointsTest(argv[1]);
  TestLoadPickedObject(argv[2]);
  TestLoadPickedPointListFromDirectoryOfMPSFiles(argv[3]);
  TestLoadMPSAndConvertToOpenCVVector ( argv[3] );
  TestLoadMPSAndConvertToOpenCVVectorWithIndexFilling ( argv[3] );
  TestInitialiseVideoCapture ( argv[4], argv[5] );
  TestCreateVideoWriter ( );
  TestReadTrackerMatrix(argv[6], argv[2]);
  MITK_TEST_END();
}



