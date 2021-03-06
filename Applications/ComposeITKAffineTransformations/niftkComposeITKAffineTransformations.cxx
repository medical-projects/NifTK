/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include <niftkConversionUtils.h>
#include <niftkCommandLineParser.h>
#include <itkAffineTransform.h>
#include <itkTransformFileWriter.h>
#include <itkTransformFileReader.h>

struct niftk::CommandLineArgumentDescription clArgList[] = {

  {OPT_SWITCH, "D2", 0, "Transformations are 2D [3D]."},

  {OPT_STRING|OPT_REQ, "i1",  "filename", "Input transformation 1."},
  {OPT_STRING|OPT_REQ, "i2", "filename", "Input transformation 2."},
  
  {OPT_STRING|OPT_REQ, "o", "filename", "Output transformation."},
  
  {OPT_DONE, NULL, NULL, 
   "Compose a pair of affine transformations "
   "(see also: niftkMultiplyTransformation)."} 
};


enum {
  O_2D=0,

  O_INPUT_FILE_1,
  O_INPUT_FILE_2, 
  
  O_OUTPUT_FILE
};

struct arguments
{
  bool flg2D;

  std::string inputFilename1;
  std::string inputFilename2;

  std::string outputFilename; 
};


template <int Dimension>
int DoMain(arguments args)
{
  typedef itk::AffineTransform<double, Dimension> AffineTransformType; 

  typename itk::TransformFileReader::TransformListType* transforms;
  
  typename AffineTransformType::Pointer inputTransform1; 
  typename AffineTransformType::Pointer inputTransform2; 

  typename AffineTransformType::MatrixType  matrix1;
  typename AffineTransformType::MatrixType  matrix2;
  
  typename AffineTransformType::Pointer    outputTransform;
  typename AffineTransformType::MatrixType matrixProduct;
  typename AffineTransformType::OffsetType offsetProduct;

  typename itk::TransformFileReader::TransformListType::const_iterator iterTransforms;


  try
  {
    typedef itk::TransformFileReader TransformFileReaderType;
    TransformFileReaderType::Pointer transformFileReader = TransformFileReaderType::New();

    // Read Transform 1

    transformFileReader->SetFileName( args.inputFilename1 );
    transformFileReader->Update();

    // Read Transform 2

    transformFileReader->SetFileName( args.inputFilename2 );
    transformFileReader->Update();

    transforms = transformFileReader->GetTransformList();
    std::cout << "Number of transforms = " << transforms->size() << std::endl;

    iterTransforms = transforms->begin();
  
    inputTransform1 = dynamic_cast<AffineTransformType*>( (*iterTransforms).GetPointer() );
    std::cout << std::endl << "Transform 1: " << std::endl;
    inputTransform1->Print( std::cout );

    matrix1 = inputTransform1->GetMatrix();
    std::cout << matrix1 << std::endl;     
  
    inputTransform2 = dynamic_cast<AffineTransformType*>( (*(++iterTransforms)).GetPointer() );
    std::cout << std::endl << "Transform 2: " << std::endl;
    inputTransform2->Print( std::cout );

    matrix2 = inputTransform2->GetMatrix();
    std::cout << matrix2 << std::endl; 


    // Compose the transformations (should handle the center properly)

    bool preCompose = false;
    inputTransform1->Compose( inputTransform2, preCompose );

    std::cout << std::endl << "Composed Transformation: " << std::endl;
    inputTransform1->Print( std::cout );


    // Write the transformation to a file

    typename itk::TransformFileWriter::Pointer transformWriter;
    transformWriter = itk::TransformFileWriter::New();

    transformWriter->SetFileName(args.outputFilename);
    transformWriter->SetInput(inputTransform1);
    transformWriter->Update();
  }  
  catch (itk::ExceptionObject& exceptionObject)
  {
    std::cerr << "ERROR: Failed compose tranformations:" << exceptionObject << std::endl;
    return EXIT_FAILURE; 
  }
  
  return EXIT_SUCCESS; 
}

  

int main(int argc, char** argv)
{
  struct arguments args;

  niftk::CommandLineParser CommandLineOptions(argc, argv, clArgList, false);
  
  CommandLineOptions.GetArgument(O_2D, args.flg2D);
  
  CommandLineOptions.GetArgument(O_INPUT_FILE_1, args.inputFilename1);
  CommandLineOptions.GetArgument(O_INPUT_FILE_2, args.inputFilename2);

  CommandLineOptions.GetArgument(O_OUTPUT_FILE, args.outputFilename);

  if ( args.flg2D )
    return DoMain<2>(args);

  else
    return DoMain<3>(args);
}
  
