/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2010-09-02 17:25:37 +0100 (Thu, 02 Sep 2010) $
 Revision          : $Revision: 6628 $
 Last modified by  : $Author: ad $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/
#ifndef SLICESELECTWIDGET_H
#define SLICESELECTWIDGET_H

#include "NifTKConfigure.h"
#include "niftkQtWin32ExportHeader.h"

#include "IntegerSpinBoxAndSliderWidget.h"

/**
 * \class SliceSelectWidget
 * \brief Creates a dockable widget to select a slice number.
 *
 * Note that the signals emitted must have globally unique names.
 * The aim is that when you adjust widgets, the signals are emitted, and
 * the only way to set the widgets are via slots.
 *
 */
class NIFTKQT_WINEXPORT SliceSelectWidget : public IntegerSpinBoxAndSliderWidget
{
  Q_OBJECT

public:

  /** Define this, so we can refer to it in map. */
  const static QString OBJECT_NAME;

  /** Default constructor. */
  SliceSelectWidget(QWidget *parent = 0);

  /** Destructor. */
  ~SliceSelectWidget();

  /** Increment slice number, where i can be positive or negative, and the result will emit signals. */
  void AddToSliceNumber(int i);

  /** Sets the current value, called from external clients. */
  void SetSliceNumber(int value);

private:

  SliceSelectWidget(const SliceSelectWidget&);  // Purposefully not implemented.
  void operator=(const SliceSelectWidget&);  // Purposefully not implemented.

};
#endif
