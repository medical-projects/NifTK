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
#include <math.h>
#include <iostream>
#include <cstdlib>
#include <mitkTestingMacros.h>
#include <mitkVector.h>
#include <mitkIOUtil.h>
#include <mitkDataStorage.h>
#include <mitkStandaloneDataStorage.h>
#include <mitkDataNode.h>

#include <niftkImageOrientation.h>
#include <niftkImageUtils.h>
#include <niftkImageOrientationUtils.h>
#include <niftkCoreObjectFactory.h>

/**
 * \brief Test class for niftkImageOrientationUtilsTest.
 */
class niftkImageOrientationUtilsTestClass
{

public:

  mitk::DataStorage::Pointer m_DataStorage;
  mitk::DataNode::Pointer m_Node;
  mitk::Image::Pointer m_Image;

  //-----------------------------------------------------------------------------
  void Setup(char* argv[])
  {
    m_DataStorage = mitk::StandaloneDataStorage::New();
    std::string fileName = argv[1];
    std::vector<std::string> files;
    files.push_back(fileName);
    mitk::DataStorage::SetOfObjects::Pointer allImages = mitk::IOUtil::Load(files, *(m_DataStorage.GetPointer()));
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(allImages->size(), 1),".. Testing 1 image loaded.");

    m_Node = (*allImages)[0];
    MITK_TEST_CONDITION_REQUIRED(m_Node.IsNotNull(),".. Testing node not null.");

    m_Image = dynamic_cast<mitk::Image*>(m_Node->GetData());
    MITK_TEST_CONDITION_REQUIRED(m_Node.IsNotNull(),".. Testing image not null.");
  }

  //-----------------------------------------------------------------------------
  void TestMitkToItk()
  {
    MITK_TEST_OUTPUT(<< "Starting TestMitkToItk...");

    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetItkOrientation(niftk::IMAGE_ORIENTATION_AXIAL),    itk::ORIENTATION_AXIAL),   ".. Testing axial.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetItkOrientation(niftk::IMAGE_ORIENTATION_SAGITTAL), itk::ORIENTATION_SAGITTAL),".. Testing sagittal.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetItkOrientation(niftk::IMAGE_ORIENTATION_CORONAL),  itk::ORIENTATION_CORONAL), ".. Testing coronal.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetItkOrientation(niftk::IMAGE_ORIENTATION_UNKNOWN),  itk::ORIENTATION_UNKNOWN), ".. Testing unknown.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetItkOrientation((niftk::ImageOrientation)999),      itk::ORIENTATION_UNKNOWN), ".. Testing garbage.");

    MITK_TEST_OUTPUT(<< "Finished TestMitkToItk...");
  }


  //-----------------------------------------------------------------------------
  void TestItkToMitk()
  {
    MITK_TEST_OUTPUT(<< "Starting TesttItkToMitk...");

    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetOrientation(itk::ORIENTATION_AXIAL),     niftk::IMAGE_ORIENTATION_AXIAL),   ".. Testing axial.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetOrientation(itk::ORIENTATION_SAGITTAL),  niftk::IMAGE_ORIENTATION_SAGITTAL),".. Testing sagittal.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetOrientation(itk::ORIENTATION_CORONAL),   niftk::IMAGE_ORIENTATION_CORONAL), ".. Testing coronal.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetOrientation(itk::ORIENTATION_UNKNOWN),   niftk::IMAGE_ORIENTATION_UNKNOWN), ".. Testing unknown.");
    MITK_TEST_CONDITION_REQUIRED(mitk::Equal(niftk::GetOrientation((itk::Orientation)999), niftk::IMAGE_ORIENTATION_UNKNOWN), ".. Testing garbage.");

    MITK_TEST_OUTPUT(<< "Finished TesttItkToMitk...");
  }


  //-----------------------------------------------------------------------------
  void TestGetOrientationString(std::string expectedOrientation)
  {
    MITK_TEST_OUTPUT(<< "Starting TestGetOrientationString...");

    std::string actualOrientation = niftk::GetOrientationString(m_Image);
    MITK_TEST_CONDITION_REQUIRED(actualOrientation == expectedOrientation, ".. Testing orientation expected=" << expectedOrientation << ", but got=" << actualOrientation);

    MITK_TEST_OUTPUT(<< "Finished TestGetOrientationString...");
  }

  //-----------------------------------------------------------------------------
  void TestGetUpDirection(int upAx, int upSa, int upCo)
  {
    MITK_TEST_OUTPUT(<< "Starting TestGetUpDirection...");

    int testAx = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_AXIAL);
    int testSa = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_SAGITTAL);
    int testCo = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_CORONAL);

    MITK_TEST_CONDITION_REQUIRED(testAx == upAx,     ".. Testing axial, up=superior=" << upAx << ", but got " << testAx);
    MITK_TEST_CONDITION_REQUIRED(testSa == upSa, ".. Testing sagittal, up=right=" << upSa << ", but got " << testSa);
    MITK_TEST_CONDITION_REQUIRED(testCo == upCo,   ".. Testing coronal, up=anterior=" << upCo << ", but got " << testCo);
    MITK_TEST_CONDITION_REQUIRED(niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_UNKNOWN) == 0,   ".. Testing unknown=0");

    MITK_TEST_OUTPUT(<< "Finished TestGetUpDirection...");
  }


  //-----------------------------------------------------------------------------
  void TestGetThroughPlaneAxis(int axAx, int axSa, int axCo)
  {
    MITK_TEST_OUTPUT(<< "Starting TestGetThroughPlaneAxis...");

    int testAx = niftk::GetThroughPlaneAxis(m_Image, niftk::IMAGE_ORIENTATION_AXIAL);
    int testSa = niftk::GetThroughPlaneAxis(m_Image, niftk::IMAGE_ORIENTATION_SAGITTAL);
    int testCo = niftk::GetThroughPlaneAxis(m_Image, niftk::IMAGE_ORIENTATION_CORONAL);

    MITK_TEST_CONDITION_REQUIRED(testAx == axAx,     ".. Testing axial==" << axAx << ", but got " << testAx);
    MITK_TEST_CONDITION_REQUIRED(testSa == axSa, ".. Testing sagittal==" << axSa << ", but got " << testSa);
    MITK_TEST_CONDITION_REQUIRED(testCo == axCo,   ".. Testing coronal==" << axCo << ", but got " << testCo);
    MITK_TEST_CONDITION_REQUIRED(niftk::GetThroughPlaneAxis(m_Image, niftk::IMAGE_ORIENTATION_UNKNOWN) == -1,   ".. Testing unknown==-1");

    MITK_TEST_OUTPUT(<< "Finished TestGetThroughPlaneAxis...");
  }


  //-----------------------------------------------------------------------------
  void TestImageUpIsSameAsGeometryUp()
  {
    MITK_TEST_OUTPUT(<< "Starting TestImageUpIsSameAsGeometryUp...");

    int imUpAx = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_AXIAL);
    int imUpSa = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_SAGITTAL);
    int imUpCo = niftk::GetUpDirection(m_Image, niftk::IMAGE_ORIENTATION_CORONAL);

    int geomUpAx = niftk::GetUpDirection(m_Image->GetGeometry(), itk::ORIENTATION_AXIAL);
    int geomUpSa = niftk::GetUpDirection(m_Image->GetGeometry(), itk::ORIENTATION_SAGITTAL);
    int geomUpCo = niftk::GetUpDirection(m_Image->GetGeometry(), itk::ORIENTATION_CORONAL);

    MITK_TEST_CONDITION_REQUIRED(imUpAx == geomUpAx,     ".. Testing axial==" << imUpAx << ", but got " << geomUpAx);
    MITK_TEST_CONDITION_REQUIRED(imUpSa == geomUpSa,     ".. Testing sagittal==" << imUpSa << ", but got " << geomUpSa);
    MITK_TEST_CONDITION_REQUIRED(imUpCo == geomUpCo,     ".. Testing coronal==" << imUpCo << ", but got " << geomUpCo);

    MITK_TEST_OUTPUT(<< "Finished TestImageUpIsSameAsGeometryUp...");
  }

};

/**
 * \brief Basic test harness for niftkImageOrientationUtils.
 */
int niftkImageOrientationUtilsTest(int argc, char * argv[])
{
  // always start with this!
  MITK_TEST_BEGIN("niftkImageOrientationUtilsTest");

  niftkImageOrientationUtilsTestClass* testClass = new niftkImageOrientationUtilsTestClass();

  std::string orientationString = argv[2];
  int upAx = atoi(argv[3]);
  int upSa = atoi(argv[4]);
  int upCo = atoi(argv[5]);
  int axAx = atoi(argv[6]);
  int axSa = atoi(argv[7]);
  int axCo = atoi(argv[8]);

  testClass->Setup(argv);
  testClass->TestMitkToItk();
  testClass->TestItkToMitk();
  testClass->TestGetOrientationString(orientationString);
  testClass->TestGetUpDirection(upAx, upSa, upCo);
  testClass->TestGetThroughPlaneAxis(axAx, axSa, axCo);
  testClass->TestImageUpIsSameAsGeometryUp();

  delete testClass;
  MITK_TEST_END();
}

