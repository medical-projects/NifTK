/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/
 
#ifndef SnapshotView_h
#define SnapshotView_h

#include <QmitkAbstractView.h>
#include "ui_SnapshotViewControls.h"


/**
 * \class SnapshotView
 * \brief Simple user interface to provide screenshots of the current editor window.
 * \ingroup uk_ac_ucl_cmic_snapshot_internal
*/
class SnapshotView : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT
  
public:

  /**
   * \brief Each View for a plugin has its own globally unique ID, this one is
   * "uk.ac.ucl.cmic.snapshot" and the .cxx file and plugin.xml should match.
   */
  static const QString VIEW_ID;

  SnapshotView();
  virtual ~SnapshotView();

protected:

  /// \brief Called by framework, this method creates all the controls for this view
  virtual void CreateQtPartControl(QWidget *parent) override;

  /// \brief Called by framework, sets the focus on a specific widget.
  virtual void SetFocus() override;

protected slots:
  
  virtual void OnTakeSnapshotButtonPressed();

protected:
  Ui::SnapshotViewControls *m_Controls;

private:

  QWidget* m_Parent;
};

#endif // SnapshotView_h

