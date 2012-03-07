/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $LastChangedDate: 2011-12-16 09:12:58 +0000 (Fri, 16 Dec 2011) $
 Revision          : $Revision: 8039 $
 Last modified by  : $Author: mjc $

 Original author   : $Author: mjc $

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#ifndef QMITKMIDASVIEWENUMS_H
#define QMITKMIDASVIEWENUMS_H

/*!
 * \file QmitkMIDASViewEnums.h
 * \brief Contains MIDAS enums, which we move out of the classes, so
 * they are independent, which makes manually analysing classes for
 * their #include dependencies a bit easier.
 */

/*!
 * \enum MIDASOrientation
 * \brief Describes the different types of orientation, axial, sagittal, coronal,
 * that can be achieved in the MIDAS style Display window. This is different from
 * the MIDASView. The orientation might be used to refer to the axis of an image,
 * so an image can ONLY be sampled in AXIAL, SAGITTAL and CORONAL direction.
 */
enum MIDASOrientation
{
  MIDAS_ORIENTATION_AXIAL = 0,
  MIDAS_ORIENTATION_SAGITTAL = 1,
  MIDAS_ORIENTATION_CORONAL = 2,
  MIDAS_ORIENTATION_UNKNOWN = 3
};

/*!
 * \enum MIDASView
 * \brief Describes the different window layouts that can be achieved in
 * the MIDAS style Display window. So one MIDASView could have
 * multiple MIDASOrientations, but most often will contain either Axial,
 * Coronal or Sagittal. This is different to the MIDASView as a view
 * can contain multiple orientations.
 */
enum MIDASView
{
  MIDAS_VIEW_AXIAL = 0,
  MIDAS_VIEW_SAGITTAL = 1,
  MIDAS_VIEW_CORONAL = 2,
  MIDAS_VIEW_ORTHO = 3,
  MIDAS_VIEW_3D = 4,
  MIDAS_VIEW_AS_ACQUIRED = 5,
  MIDAS_VIEW_UNKNOWN = 6
};

/*!
 * \enum MIDASDropType
 * \brief Describes the different modes that can be used when drag and dropping
 * into the MIDAS style Display window.
 */
enum MIDASDropType
{
  MIDAS_DROP_TYPE_SINGLE = 0,   /** This means that multiple nodes are dropped into a single window. */
  MIDAS_DROP_TYPE_MULTIPLE = 1, /** This means that multiple nodes are dropped across multiple windows. */
  MIDAS_DROP_TYPE_ALL = 2,      /** This means that multiple nodes are dropped across all windows for a thumnail effect. */
};

/*!
 * \enum MIDASDefaultInterpolationType
 * \brief Describes what the interpolation type should be set to when an image is dropped.
 */
enum MIDASDefaultInterpolationType
{
  MIDAS_INTERPOLATION_NONE,
  MIDAS_INTERPOLATION_LINEAR,
  MIDAS_INTERPOLATION_CUBIC
};

#endif
