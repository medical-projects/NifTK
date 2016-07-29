/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include <itkLogHelper.h>
#include <niftkConversionUtils.h>
#include <vtkDataObject.h>
#include <vtkStructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkStructuredGridReader.h>
#include <vtkStructuredPointsReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkContourFilter.h>
#include <vtkMarchingCubes.h>
#include <NifTKConfigure.h>
#include <vtkPolyDataNormals.h>
 #include <vtkSmartPointer.h>


/*!
 * \file niftkMarchingCubes.cxx
 * \page niftkMarchingCubes
 * \section niftkMarchingCubesSummary Takes an image as a VTK structured grid (or structured points), and performs a marching cubes iso-surface contour extraction.
 */

void Usage(char *exec)
  {
    niftk::itkLogHelper::PrintCommandLineHeader(std::cout);
    std::cout << "  " << std::endl;
    std::cout << "  Takes an image as a VTK structured grid (NOT structured points), and performs a marching cubes iso-surface extraction." << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "  " << exec << " -i inputImage.vtk -o outputPolyData.vtk [options]" << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "*** [mandatory] ***" << std::endl << std::endl;
    std::cout << "    -i    <filename>      Input image as VTK structured grid" << std::endl;
    std::cout << "    -o    <filename>      Output VTK Poly Data" << std::endl << std::endl;      
    std::cout << "*** [options]   ***" << std::endl << std::endl;
    std::cout << "    -iso  <float> [128]   Threshold value to extract" << std::endl;
    std::cout << "    -points               Input is structured points [structure grid]" << std::endl;
    std::cout << "    -withScalars          Computes scalar values for each vertex" << std::endl;
    std::cout << "    -withNormals          Computes normals at each vertex" << std::endl;
    std::cout <<"     -withGradient         Computes gradient at each vertex" << std::endl;
  }

struct arguments
{
  std::string inputImage;
  std::string outputPolyData;

  float isoSurfaceValue;

  bool isStructuredPoints;
  bool withScalars;
  bool withGradients;
  bool withNormals;
};

/**
 * \brief Runs marching cubes.
 */
int main(int argc, char** argv)
{
  // To pass around command line args
  struct arguments args;

  args.isoSurfaceValue = 128;
  args.isStructuredPoints = false;
  args.withGradients = false;
  args.withScalars = false;
  args.withNormals = false;
  

  // Parse command line args
  for(int i=1; i < argc; i++){
    if(strcmp(argv[i], "-help")==0 || strcmp(argv[i], "-Help")==0 || strcmp(argv[i], "-HELP")==0 || strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--h")==0){
      Usage(argv[0]);
      return -1;
    }
    else if(strcmp(argv[i], "-i") == 0){
      args.inputImage=argv[++i];
      std::cout << "Set -i=" << args.inputImage << std::endl;
    }
    else if(strcmp(argv[i], "-o") == 0){
      args.outputPolyData=argv[++i];
      std::cout << "Set -o=" << args.outputPolyData << std::endl;
    }
    else if(strcmp(argv[i], "-iso") == 0){
      args.isoSurfaceValue=atof(argv[++i]);
      std::cout << "Set -iso=" << niftk::ConvertToString(args.isoSurfaceValue) << std::endl;
    }
    else if(strcmp(argv[i], "-points") == 0){
      args.isStructuredPoints=true;
      std::cout << "Set -points=" << niftk::ConvertToString(args.isStructuredPoints) << std::endl;
    }
    else if(strcmp(argv[i], "-withScalars") == 0){
      args.withScalars=true;
      std::cout << "Set -withScalars=" << niftk::ConvertToString(args.withScalars) << std::endl;
    }
    else if(strcmp(argv[i], "-withGradients") == 0){
      args.withGradients=true;
      std::cout << "Set -withGradients=" << niftk::ConvertToString(args.withGradients) << std::endl;
    }
    else if(strcmp(argv[i], "-withNormals") == 0){
      args.withNormals=true;
      std::cout << "Set -withNormals=" << niftk::ConvertToString(args.withNormals) << std::endl;
    }
    else {
      std::cerr << argv[0] << ":\tParameter " << argv[i] << " unknown." << std::endl;
      return -1;
    }            
  }
  
  // Validate command line args
  if (args.inputImage.length() == 0 || args.outputPolyData.length() == 0)
    {
      Usage(argv[0]);
      return EXIT_FAILURE;
    }



  // Read structured points
  // ~~~~~~~~~~~~~~~~~~~~~~
  
  if ( args.isStructuredPoints )
  {

    vtkSmartPointer<vtkStructuredPointsReader> 
      reader = vtkSmartPointer<vtkStructuredPointsReader>::New();

    reader->SetFileName(args.inputImage.c_str());

    reader->Update();
     
  
    // Create the marching cubes filter

    vtkSmartPointer<vtkMarchingCubes> 
      filter = vtkSmartPointer<vtkMarchingCubes>::New();

    filter->SetInputConnection( reader->GetOutputPort() );

    filter->SetValue( 0, args.isoSurfaceValue );

    filter->SetComputeScalars( args.withScalars );
    filter->SetComputeGradients( args.withGradients );
    filter->SetComputeNormals( args.withNormals );
  
    filter->Update();


    // Write the ouput

    vtkSmartPointer<vtkPolyDataWriter> 
      writer = vtkSmartPointer<vtkPolyDataWriter>::New();

    writer->SetInputDataObject( filter->GetOutput() );
    writer->SetFileName( args.outputPolyData.c_str() );

    writer->Update();
  }


  // Read structured grid
  // ~~~~~~~~~~~~~~~~~~~~
  
  else
  {

    vtkSmartPointer<vtkStructuredGridReader>
      reader = vtkSmartPointer<vtkStructuredGridReader>::New();
    reader->SetFileName(args.inputImage.c_str());

    reader->Update();
     

    // Create the marching cubes filter

    vtkSmartPointer<vtkContourFilter>
      filter = vtkSmartPointer<vtkContourFilter>::New();

    filter->SetInputDataObject( reader->GetOutput() );

    filter->SetValue( 0, args.isoSurfaceValue );

    filter->SetComputeScalars( args.withScalars );
    filter->SetComputeGradients( args.withGradients );
    filter->SetComputeNormals( args.withNormals );
  
    filter->Update();


    // Compute normals

    vtkSmartPointer<vtkPolyDataNormals>
      normals = vtkSmartPointer<vtkPolyDataNormals>::New();

    normals->SetInputDataObject( filter->GetOutput() );
    normals->SetFeatureAngle(60.0);

    normals->Update();


    // Write the ouput

    vtkSmartPointer<vtkPolyDataWriter>
      writer = vtkSmartPointer<vtkPolyDataWriter>::New();

    writer->SetInputDataObject( normals->GetOutput() );
    writer->SetFileName( args.outputPolyData.c_str() );

    writer->Update();
  }

}