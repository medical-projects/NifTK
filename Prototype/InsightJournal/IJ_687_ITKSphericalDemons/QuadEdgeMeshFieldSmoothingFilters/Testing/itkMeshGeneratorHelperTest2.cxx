/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageRegistration8.cxx,v $
  Language:  C++
  Date:      $Date: 2010-05-26 10:55:12 +0100 (Wed, 26 May 2010) $
  Version:   $Revision: 3302 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "itkMeshGeneratorHelper2.h"
#include "itkMeshWriterHelper2.h"

int main( int argc, char *argv[] )
{
  if( argc < 2 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " outputMeshFile ";
    return EXIT_FAILURE;
    }
  
  const unsigned int Dimension = 3;
  typedef itk::Vector< double, Dimension > MeshPixelType;

  typedef itk::QuadEdgeMesh< MeshPixelType, Dimension >   MeshType;

  typedef itk::MeshGeneratorHelper2< MeshType >   GeneratorType;

  MeshType::Pointer        mesh;

  GeneratorType::GenerateMesh( mesh );

  itk::MeshWriterHelper2< MeshType >::WriteMeshToFile( mesh, argv[1] );

  return EXIT_SUCCESS;
}