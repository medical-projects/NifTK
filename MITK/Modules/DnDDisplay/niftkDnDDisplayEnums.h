/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkDnDDisplayEnums_h
#define niftkDnDDisplayEnums_h


namespace niftk
{

/// \enum WindowOrientation
/// \brief Describes the different types of orientation, axial, sagittal, coronal,
/// that can be achieved in the Drag&Drop Display windows. This is different from
/// the WindowLayout. The orientation might be used to refer to the axis of an image,
/// so an image can ONLY be sampled in AXIAL, SAGITTAL and CORONAL direction.
enum WindowOrientation
{
  WINDOW_ORIENTATION_AXIAL = 0,
  WINDOW_ORIENTATION_SAGITTAL = 1,
  WINDOW_ORIENTATION_CORONAL = 2,
  WINDOW_ORIENTATION_UNKNOWN = 3
};

/// \brief The number of the possible orientations.
const int WINDOW_ORIENTATION_NUMBER = 4;


/// \enum DisplayConvention
/// \brief The display convention determines the view point and whether the slice
///  is mirrored or rotated on the display.
///
/// 'Top', 'frontside' and 'rotated' refers to the parameters of `mitk::SlicedGeometry3D::Update()`.
///
/// If 'top' is false then the z axis is flipped. If 'frontside' is false then
/// the x axis is flipped. If 'rotated' is true then the x and the y axes is
/// flipped. There is no flag to flip only the y axis, but `frontsize == false`
/// and `rotated == true` together has the same effect.
// There is a gap between the flag value ranges of the individual orientations,
// in case new flags would be introduced later, e.g. for swapping the x and y
// axes that is currently not possible.
enum DisplayConvention
{
  DISPLAY_CONVENTION_SAGITTAL_TOP = 1 << 0,
  DISPLAY_CONVENTION_SAGITTAL_FRONTSIDE = 1 << 1,
  DISPLAY_CONVENTION_SAGITTAL_ROTATED = 1 << 2,
  DISPLAY_CONVENTION_CORONAL_TOP = 1 << 8,
  DISPLAY_CONVENTION_CORONAL_FRONTSIDE = 1 << 9,
  DISPLAY_CONVENTION_CORONAL_ROTATED = 1 << 10,
  DISPLAY_CONVENTION_AXIAL_TOP = 1 << 16,
  DISPLAY_CONVENTION_AXIAL_FRONTSIDE = 1 << 17,
  DISPLAY_CONVENTION_AXIAL_ROTATED = 1 << 18,
};

const int DISPLAY_CONVENTION_RADIO =
    DISPLAY_CONVENTION_SAGITTAL_TOP | DISPLAY_CONVENTION_SAGITTAL_FRONTSIDE |
    DISPLAY_CONVENTION_CORONAL_FRONTSIDE |
    DISPLAY_CONVENTION_AXIAL_ROTATED;

const int DISPLAY_CONVENTION_NEURO =
    DISPLAY_CONVENTION_SAGITTAL_TOP | DISPLAY_CONVENTION_SAGITTAL_FRONTSIDE |
    DISPLAY_CONVENTION_CORONAL_TOP |
    DISPLAY_CONVENTION_AXIAL_TOP | DISPLAY_CONVENTION_AXIAL_FRONTSIDE | DISPLAY_CONVENTION_AXIAL_ROTATED;

const int DISPLAY_CONVENTION_RADIO_X_FLIPPED =
    DISPLAY_CONVENTION_SAGITTAL_FRONTSIDE |
    DISPLAY_CONVENTION_AXIAL_FRONTSIDE | DISPLAY_CONVENTION_AXIAL_ROTATED;


/// \enum WindowLayout
/// \brief Describes the different render window layouts. So one WindowLayout could have
/// multiple windows of different orientations, but most often will contain either axial,
/// coronal or sagittal. This is different to the WindowLayout as a layout
/// can contain multiple orientations.
enum WindowLayout
{
  WINDOW_LAYOUT_AXIAL = 0,
  WINDOW_LAYOUT_SAGITTAL = 1,
  WINDOW_LAYOUT_CORONAL = 2,
  WINDOW_LAYOUT_ORTHO = 3,
  WINDOW_LAYOUT_ORTHO_NO_3D = 4,
  WINDOW_LAYOUT_3D = 5,
  WINDOW_LAYOUT_3H = 6,
  WINDOW_LAYOUT_3V = 7,
  WINDOW_LAYOUT_AS_ACQUIRED = 8,
  WINDOW_LAYOUT_UNKNOWN = 9,
  WINDOW_LAYOUT_COR_SAG_H = 10,
  WINDOW_LAYOUT_COR_SAG_V = 11,
  WINDOW_LAYOUT_COR_AX_H = 12,
  WINDOW_LAYOUT_COR_AX_V = 13,
  WINDOW_LAYOUT_SAG_AX_H = 14,
  WINDOW_LAYOUT_SAG_AX_V = 15
};


/// \brief Returns true if the layout contains only one window, otherwise false.
inline bool IsSingleWindowLayout(WindowLayout layout)
{
  return layout == WINDOW_LAYOUT_AXIAL
      || layout == WINDOW_LAYOUT_SAGITTAL
      || layout == WINDOW_LAYOUT_CORONAL
      || layout == WINDOW_LAYOUT_3D;
}


/// \brief Returns true if the layout contains multiple windows, otherwise false.
inline bool IsMultiWindowLayout(WindowLayout layout)
{
  return !IsSingleWindowLayout(layout);
}

/// \brief The number of the possible window layouts.
const int WINDOW_LAYOUT_NUMBER = 16;

/// \brief Gets the window layout number from the name of the window layout.
inline WindowLayout GetWindowLayout(const std::string& windowLayoutName)
{
  WindowLayout windowLayout = WINDOW_LAYOUT_UNKNOWN;

  if (windowLayoutName == "axial")
  {
    windowLayout = WINDOW_LAYOUT_AXIAL;
  }
  else if (windowLayoutName == "sagittal")
  {
    windowLayout = WINDOW_LAYOUT_SAGITTAL;
  }
  else if (windowLayoutName == "coronal")
  {
    windowLayout = WINDOW_LAYOUT_CORONAL;
  }
  else if (windowLayoutName == "2x2")
  {
    windowLayout = WINDOW_LAYOUT_ORTHO;
  }
  else if (windowLayoutName == "2x2 no 3D")
  {
    windowLayout = WINDOW_LAYOUT_ORTHO_NO_3D;
  }
  else if (windowLayoutName == "3D")
  {
    windowLayout = WINDOW_LAYOUT_3D;
  }
  else if (windowLayoutName == "3H")
  {
    windowLayout = WINDOW_LAYOUT_3H;
  }
  else if (windowLayoutName == "3V")
  {
    windowLayout = WINDOW_LAYOUT_3V;
  }
  else if (windowLayoutName == "as acquired")
  {
    windowLayout = WINDOW_LAYOUT_AS_ACQUIRED;
  }
  else if (windowLayoutName == "cor sag H")
  {
    windowLayout = WINDOW_LAYOUT_COR_SAG_H;
  }
  else if (windowLayoutName == "cor sag V")
  {
    windowLayout = WINDOW_LAYOUT_COR_SAG_V;
  }
  else if (windowLayoutName == "cor ax H")
  {
    windowLayout = WINDOW_LAYOUT_COR_AX_H;
  }
  else if (windowLayoutName == "cor ax V")
  {
    windowLayout = WINDOW_LAYOUT_COR_AX_V;
  }
  else if (windowLayoutName == "sag ax H")
  {
    windowLayout = WINDOW_LAYOUT_SAG_AX_H;
  }
  else if (windowLayoutName == "sag ax V")
  {
    windowLayout = WINDOW_LAYOUT_SAG_AX_V;
  }

  return windowLayout;
}


/// \brief Tells if a window with a given index belongs to a given window layout.
/// The index can go from 0 to 3 and these values mean the axial, sagittal, coronal
/// or 3D windows, in this order. If the index is outside of the range, false is returned.
inline bool IsWindowVisibleInLayout(int windowIndex, WindowLayout windowLayout)
{
  return
      windowIndex == 0 ?
          windowLayout == WINDOW_LAYOUT_AXIAL
          || windowLayout == WINDOW_LAYOUT_3H
          || windowLayout == WINDOW_LAYOUT_3V
          || windowLayout == WINDOW_LAYOUT_ORTHO
          || windowLayout == WINDOW_LAYOUT_ORTHO_NO_3D
          || windowLayout == WINDOW_LAYOUT_COR_AX_H
          || windowLayout == WINDOW_LAYOUT_COR_AX_V
          || windowLayout == WINDOW_LAYOUT_SAG_AX_H
          || windowLayout == WINDOW_LAYOUT_SAG_AX_V
      : windowIndex == 1 ?
          windowLayout == WINDOW_LAYOUT_SAGITTAL
          || windowLayout == WINDOW_LAYOUT_3H
          || windowLayout == WINDOW_LAYOUT_3V
          || windowLayout == WINDOW_LAYOUT_ORTHO
          || windowLayout == WINDOW_LAYOUT_ORTHO_NO_3D
          || windowLayout == WINDOW_LAYOUT_COR_SAG_H
          || windowLayout == WINDOW_LAYOUT_COR_SAG_V
          || windowLayout == WINDOW_LAYOUT_SAG_AX_H
          || windowLayout == WINDOW_LAYOUT_SAG_AX_V
      : windowIndex == 2 ?
          windowLayout == WINDOW_LAYOUT_CORONAL
          || windowLayout == WINDOW_LAYOUT_3H
          || windowLayout == WINDOW_LAYOUT_3V
          || windowLayout == WINDOW_LAYOUT_ORTHO
          || windowLayout == WINDOW_LAYOUT_ORTHO_NO_3D
          || windowLayout == WINDOW_LAYOUT_COR_SAG_H
          || windowLayout == WINDOW_LAYOUT_COR_SAG_V
          || windowLayout == WINDOW_LAYOUT_COR_AX_H
          || windowLayout == WINDOW_LAYOUT_COR_AX_V
      : windowIndex == 3 ?
          windowLayout == WINDOW_LAYOUT_3D
          || windowLayout == WINDOW_LAYOUT_ORTHO
      : false;
}


/// \enum DnDDisplayDropType
/// \brief Describes the different modes that can be used when drag and dropping
/// into the DnD Display window.
enum DnDDisplayDropType
{
  /// This means that multiple nodes are dropped into a single window.
  DNDDISPLAY_DROP_SINGLE = 0,
  /// This means that multiple nodes are dropped across multiple windows.
  DNDDISPLAY_DROP_MULTIPLE = 1,
  /// This means that multiple nodes are dropped across all windows for a thumbnail effect.
  DNDDISPLAY_DROP_ALL = 2
};


/// \enum DnDDisplayInterpolationType
/// \brief Describes what the interpolation type should be set to when an image is dropped.
enum DnDDisplayInterpolationType
{
  DNDDISPLAY_NO_INTERPOLATION,
  DNDDISPLAY_LINEAR_INTERPOLATION,
  DNDDISPLAY_CUBIC_INTERPOLATION
};

}

#endif
