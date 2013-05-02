/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef QMITKMIDASSLIDERSWIDGET_H
#define QMITKMIDASSLIDERSWIDGET_H

#include <niftkMIDASGuiExports.h>
#include "ui_QmitkMIDASSlidersWidget.h"

/**
 * \class QmitkMIDASSlidersWidget
 * \brief Qt Widget class to contain sliders for slice, time and magnification.
 */
class NIFTKMIDASGUI_EXPORT QmitkMIDASSlidersWidget : public QWidget, public Ui_QmitkMIDASSlidersWidget
{
  // this is needed for all Qt objects that should have a MOC object (everything that derives from QObject)
  Q_OBJECT

public:

  QmitkMIDASSlidersWidget(QWidget *parent = 0);

  /** Destructor. */
  ~QmitkMIDASSlidersWidget();

  /// \brief Creates the GUI.
  void setupUi(QWidget*);

  /// \brief Calls blockSignals(block) on all contained widgets.
  bool BlockSignals(bool block);

  /// \brief Set whether the slice selection ctkSlidersWidget is tracking.
  void SetSliceTracking(bool isTracking);

  /// \brief Set whether the magnification selection ctkSlidersWidget is tracking.
  void SetMagnificationTracking(bool isTracking);

  /// \brief Set whether the time selection ctkSlidersWidget is tracking.
  void SetTimeTracking(bool isTracking);

signals:

protected slots:

protected:

private:

};

#endif
