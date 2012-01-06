/*
 * Author:  Bjoern Eiben
 * Purpose: Deform an image which was generated by niftySim
 * Date:    9th Nov. 2011
 *
 */


/*
 * includes
 */
#include <stdlib.h>

/* itk */
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkVector.h"
#include "itkWarpImageFilter.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkImageAdaptor.h"
#include "itkResampleImageFilter.h"
#include "itkMaskImageFilter.h"
#include "itkCastImageFilter.h"

/* niftk */
#include "CommandLineParser.h"

#include "itkNiftySimGravityTransformation.h"





/*
 * Command line parser options
 */
struct niftk::CommandLineArgumentDescription clArgList[] =
{
	{ OPT_STRING | OPT_REQ, "i",           "filename",       "Input image (short pixel type assumed)."                              },
	{ OPT_STRING | OPT_REQ, "o",           "filename",       "Output image (short)."                                                },
	{ OPT_STRING,           "iL",          "filename",       "Label image to be deformed (unsigned chartype assumed)."              },
	{ OPT_STRING,           "oL",          "filename",       "Label output image (short, nearest neighbourgh will be used)."        },
	{ OPT_STRING | OPT_REQ, "x",           "filename",       "NifytSimxml file name."                                               },
	{ OPT_STRING | OPT_REQ, "interpolate", "nn|lin|bspl",    "Interpolation scheme used (nn, lin, or bspl)"                         },
	{ OPT_STRING,           "m",           "filename",       "Output mask image name"                                               },
	{ OPT_INT,              "mval",        "int mask value", "Defines the pixel value of the resampled image outside the model [0]" },
	{ OPT_DONE, NULL, NULL,                                  "Warps the input image by niftySim simluation specified."              },
};



enum
{
    O_INPUT = 0,
	O_OUTPUT,
	O_INPUT_L,
	O_OUTPUT_L,
    O_XMLFILE,
	O_INTERPOLATION,
	O_MASKIMAGENAME,
	O_MASKVALUE,
};

enum
{
	NEAREST_NEIGHBOUR = 0,
	LINEAR,
	BSPLINE,
} eInterpolation;





int main(int argc, char ** argv)
{
	const unsigned int Dimension = 3;

	typedef double                                             VectorComponentType;
	typedef short                                              InputPixelType;
	typedef short                                              OutputPixelType;
	typedef itk::Vector<VectorComponentType, Dimension>        VectorType;
	typedef itk::Image<InputPixelType, Dimension>              InputImageType;
	typedef InputImageType::Pointer                            InputImagePointerType;

	typedef itk::Image<OutputPixelType, Dimension>             OutputImageType;
	typedef OutputImageType::Pointer                           OutputImagePointerType;

	typedef itk::NiftySimGravityTransformation< InputImageType, 
										        VectorComponentType, 
									            Dimension, 
										        VectorComponentType >  NiftySimTransformationType;
	
	typedef NiftySimTransformationType::DeformationFieldMaskType       DeformationFieldMaskType;
	typedef NiftySimTransformationType::Pointer                        NiftySimTransformationPointerType;
	typedef NiftySimTransformationType::ParametersType                 TrafoParametersType;

	/*
	 * Get the user-input
	 */

	char*       pcXMLFileName;
	std::string strInputImageName;
	std::string strOutputImageName;
	std::string strInputImageNameLabel;
	std::string strOutputImageNameLabel;
	std::string strGivenInterpolation;
	std::string strMaskImageName;
	int         iOutsideVal=0;


    niftk::CommandLineParser CommandLineOptions( argc, argv, clArgList, true );
    CommandLineOptions.GetArgument( O_INPUT,         strInputImageName       );
    CommandLineOptions.GetArgument( O_OUTPUT,        strOutputImageName      );
	CommandLineOptions.GetArgument( O_INPUT_L,       strInputImageNameLabel  );
	CommandLineOptions.GetArgument( O_OUTPUT_L,      strOutputImageNameLabel );
	CommandLineOptions.GetArgument( O_XMLFILE,       pcXMLFileName           );
	CommandLineOptions.GetArgument( O_INTERPOLATION, strGivenInterpolation   );
	CommandLineOptions.GetArgument( O_MASKIMAGENAME, strMaskImageName        );
	CommandLineOptions.GetArgument( O_MASKVALUE,     iOutsideVal             );

	/*
	 * Read the images
	 */

	/* input image */
    typedef itk::ImageFileReader< InputImageType >   InputImageReaderType;
    typedef InputImageReaderType::Pointer            InputImageReaderPointerType;

    InputImageReaderPointerType inputReader = InputImageReaderType::New();
    inputReader->SetFileName( strInputImageName );

	try
	{
		inputReader->Update();
	}
	catch(itk::ExceptionObject e)
	{
		std::cerr << "Could not read input image: " << strInputImageName << std::endl;
	}

	NiftySimTransformationPointerType simTrafo = NiftySimTransformationType::New();

	TrafoParametersType params = simTrafo->GetParameters();
	params.fill( 0. );
	simTrafo->SetxmlFName( pcXMLFileName );
	simTrafo->SetsportMode( true );
	simTrafo->SetVerbose( true );
	simTrafo->Initialize( inputReader->GetOutput() ); // souce and target image are supposed to be the same for now
	simTrafo->SetParameters( params );

	
	typedef itk::ResampleImageFilter< InputImageType, OutputImageType >  ResampleImageFilterType;
	typedef ResampleImageFilterType::Pointer                             ResampleImageFilterPointerType;

	ResampleImageFilterPointerType  resampler = ResampleImageFilterType::New();
	resampler->SetTransform( simTrafo );
	resampler->SetInput( inputReader->GetOutput() );
	resampler->SetUseReferenceImage(false);

	resampler->SetOutputParametersFromImage(inputReader->GetOutput());

	/*
	 * Which Interpolation to use...
	 */
	if ( strGivenInterpolation.compare("nn") == 0 ) 
	{
		eInterpolation = NEAREST_NEIGHBOUR;
		std::cout << "Using nearest neighbor interpolation." << std::endl;

	}
	else if ( strGivenInterpolation.compare("lin") == 0 )
	{
		eInterpolation = LINEAR;
		std::cout << "Using nearest neighbor interpolation." << std::endl;
	}
	else if ( strGivenInterpolation.compare("bspl") == 0 )
	{
		eInterpolation = BSPLINE;
		std::cout << "Using b-spline interpolation." << std::endl;
	}else
	{
		eInterpolation = BSPLINE;
		std::cout << "Using default b-spline interpolation." << std::endl;
	}

    /* create the interpolators */
    typedef itk::BSplineInterpolateImageFunction< InputImageType, double >  BSplineInterpolatorType;
	typedef BSplineInterpolatorType::Pointer                                BSplineInterpolatorPointerType;
    BSplineInterpolatorPointerType bSplineInterpolator = BSplineInterpolatorType::New();
	bSplineInterpolator->SetSplineOrder( 3u );

	typedef itk::LinearInterpolateImageFunction< InputImageType, double >  LinearInterpolatorType;
	typedef LinearInterpolatorType::Pointer                                LinearInterpolatorPointerType;
	LinearInterpolatorPointerType linearInterpolator = LinearInterpolatorType::New();

	typedef itk::NearestNeighborInterpolateImageFunction< InputImageType, double >  NearestNeighborInterpolatorType;
	typedef NearestNeighborInterpolatorType::Pointer                                NearestNeighborInterpolatorPointerType;
	NearestNeighborInterpolatorPointerType nnInterpolator = NearestNeighborInterpolatorType::New();
	
	// Set the correct interpolation type
	if ( eInterpolation == NEAREST_NEIGHBOUR )
	{
		resampler->SetInterpolator( nnInterpolator );
	}
	else if (eInterpolation == LINEAR )
	{
		resampler->SetInterpolator( linearInterpolator );
	}else
	{
		resampler->SetInterpolator( bSplineInterpolator );
	}

	/*
	 * Mask the output of the deformed image as the "outside" does not make sense...
	 */
	typedef itk::MaskImageFilter< OutputImageType, 
		                          DeformationFieldMaskType, 
								  OutputImageType >           MaskFilterType;
	
	MaskFilterType::Pointer maskFilter = MaskFilterType::New();
	maskFilter->SetInput1( resampler->GetOutput() );
	maskFilter->SetInput2( simTrafo->GetDeformationFieldMask() );
	maskFilter->SetOutsideValue( iOutsideVal );

	/*
	 * Write the result
	 */
    typedef itk::ImageFileWriter< OutputImageType >  ImageWriterType;
    typedef ImageWriterType::Pointer                 ImageWriterPointerType;

    ImageWriterPointerType imageWriter = ImageWriterType::New();
	imageWriter->SetInput   ( maskFilter->GetOutput() );
    imageWriter->SetFileName( strOutputImageName      );

    try
    {
    	imageWriter->Update();
	}
    catch ( itk::ExceptionObject e )
	{
		std::cout << "Something went terribly wrong..." << std::endl << e << std::endl;
    	return EXIT_FAILURE;
	}
	
	/*
	 * Write the mask image
	 */
	if ( ! strMaskImageName.empty() )
	{
		typedef itk::ImageFileWriter< DeformationFieldMaskType > MaskWriterType;
		
		MaskWriterType::Pointer maskWriter = MaskWriterType::New();
		maskWriter->SetFileName( strMaskImageName );
		maskWriter->SetInput( simTrafo->GetDeformationFieldMask() );

		try
		{
			maskWriter->Update();
		}
		catch( itk::ExceptionObject e )
		{
			std::cout << "Something went terribly wrong..." << std::endl << e << std::endl;
    		return EXIT_FAILURE;
		}
	}


	/*
	 * Deform the second image if required
	 */
	if ( (! strInputImageNameLabel.empty() ) && (! strOutputImageNameLabel.empty() ) )
	{
		// To avoid running the simulation again, the label image is read as 
		// short and thereafter casted into unsigned char

		std::cout << "Resampling second image" << std::endl;

		typedef unsigned char							 LabelPixelType;
		typedef itk::Image<LabelPixelType, Dimension >   LabelImageType;
		
		inputReader->SetFileName( strInputImageNameLabel );
		
		resampler->SetInput       ( inputReader->GetOutput() );
		resampler->SetInterpolator( nnInterpolator           );
		resampler->SetTransform   ( simTrafo                 );

		typedef itk::CastImageFilter< InputImageType, LabelImageType > CastFilterType;
		typedef CastFilterType::Pointer CastFilterPointerType;
		CastFilterPointerType caster = CastFilterType::New();

		caster->SetInput( resampler->GetOutput() );

		typedef itk::ImageFileWriter< LabelImageType > LabelWriterType;
		typedef LabelWriterType::Pointer LabelWriterPointerType;

		LabelWriterPointerType labelWriter = LabelWriterType::New();
		labelWriter->SetInput( caster->GetOutput() );
		labelWriter->SetFileName( strOutputImageNameLabel );

		try	
		{
    		labelWriter->Update();
		}
		catch ( itk::ExceptionObject e )
		{
			std::cout << "Something went terribly wrong when writing label image..." << std::endl << e << std::endl;
    		return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
