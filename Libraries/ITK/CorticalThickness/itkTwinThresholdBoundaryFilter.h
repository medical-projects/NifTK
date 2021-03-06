/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef itkTwinThresholdBoundaryFilter_h
#define itkTwinThresholdBoundaryFilter_h

#include <itkImageToImageFilter.h>

namespace itk {

/*
 * \class TwinThresholdBoundaryFilter
 * \brief Takes two inputs, if input 1 above threshold 1 and at least one
 * voxel in the 8 (2D), or 26 (3D) neighbourhood of image 2 is above threshold 2,
 * then output is True, otherwise False.
 * 
 * This filter was written to extract the grey/white interface (GWI) mentioned
 * in Das et. al. NeuroImage 45 (2009) 867-879.  So, if input 1 is a grey matter
 * PV map, and input 2 is a white matter PV map, then we can extract a boundary
 * where the grey matter probability is > 0.5 and the white matter probability of
 * at least one connected voxel is greater than 0.5.
 * 
 * We can set the thresholds that are tested eg. ThresholdForInput1 and ThresholdForInput2
 * and also the values that are set, True=1, False=0.
 */
template< class TImageType>
class ITK_EXPORT TwinThresholdBoundaryFilter : public ImageToImageFilter<TImageType, TImageType>
{
  public:
    // Standard ITK style typedefs.
    typedef TwinThresholdBoundaryFilter                Self;
    typedef ImageToImageFilter<TImageType, TImageType> Superclass;
    typedef SmartPointer<Self>                         Pointer;
    typedef SmartPointer<const Self>                   ConstPointer;

    // Additional typedefs
    typedef TImageType                                 ImageType;
    typedef typename ImageType::Pointer                ImagePointer;
    typedef typename TImageType::PixelType             PixelType;
    
    /** Method for creation through object factory */
    itkNewMacro(Self);

    /** Run-time type information */
    itkTypeMacro(TwinThresholdBoundaryFilter, ImageToImageFilter);

    /** Get the number of dimensions we are working in. */
    itkStaticConstMacro(Dimension, unsigned int, TImageType::ImageDimension);

    /** Sets input 1. */
    void SetInput1(ImagePointer image) { this->SetInput(0, image); }

    /** Sets input 2. */
    void SetInput2(ImagePointer image) { this->SetInput(1, image); }

    /** Set/Get threshold 1. */
    itkSetMacro(ThresholdForInput1, PixelType);
    itkGetMacro(ThresholdForInput1, PixelType);

    /** Set/Get threshold 2. */
    itkSetMacro(ThresholdForInput2, PixelType);
    itkGetMacro(ThresholdForInput2, PixelType);

    /** Set/Get the true value (i.e. we are on boundary). */
    itkSetMacro(True, PixelType);
    itkGetMacro(True, PixelType);

    /** Set/Get the false value (i.e. we are on boundary). */
    itkSetMacro(False, PixelType);
    itkGetMacro(False, PixelType);

    /** Set/Get whether we are doing FullyConnected or not. */
    itkSetMacro(FullyConnected, bool);
    itkGetMacro(FullyConnected, bool);

  protected:

    TwinThresholdBoundaryFilter();
    ~TwinThresholdBoundaryFilter();
    void GenerateData();
    void PrintSelf(std::ostream& os, Indent indent) const;
    
  private:
    
    TwinThresholdBoundaryFilter(Self&);  // intentionally not implemented
    void operator=(const Self&);         // intentionally not implemented

    PixelType m_ThresholdForInput1;
    PixelType m_ThresholdForInput2;
    
    PixelType m_True;
    PixelType m_False;
    
    /** If true, we do fully connected neighbours (8 in 2D, 26 in 3D), if false, we do 4 in 2D and 6 in 3D. Default true. */
    bool m_FullyConnected;

}; // end class

} // end namespace

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkTwinThresholdBoundaryFilter.txx"
#endif

#endif
