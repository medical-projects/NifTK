/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkCaffeSegmentorView_h
#define niftkCaffeSegmentorView_h

#include <niftkBaseView.h>
#include <service/event/ctkEvent.h>

namespace niftk
{

class CaffeSegController;

/// \class CaffeSegmentorView
/// \brief Provides a test view for Caffe Segmentation.
///
/// \sa niftkBaseView
/// \sa CaffeSegmentorController
class CaffeSegmentorView : public BaseView
{
  Q_OBJECT

public:

  /// \brief Each View for a plugin has its own globally unique ID, this one is
  /// "uk.ac.ucl.cmic.caffesegmentor" and the .cxx file and plugin.xml should match.
  static const QString VIEW_ID;

  /// \brief Constructor.
  CaffeSegmentorView();

  /// \brief Copy constructor which deliberately throws a runtime exception, as no-one should call it.
  CaffeSegmentorView(const CaffeSegmentorView& other);

  /// \brief Destructor.
  virtual ~CaffeSegmentorView();

protected:

  /// \brief Creates the GUI parts.
  virtual void CreateQtPartControl(QWidget* parent) override;

  /// \brief Called by framework, this method can set the focus on a specific widget,
  /// but we currently do nothing.
  virtual void SetFocus() override;

  /// \brief Retrieve's the pref values from preference service, and store locally.
  virtual void RetrievePreferenceValues();

private slots:

  /// \brief We listen to the event bus to trigger updates.
  void OnUpdate(const ctkEvent& event);

private:

  /// \brief Called when preferences are updated.
  virtual void OnPreferencesChanged(const berry::IBerryPreferences*) override;

  /// \brief The Caffe segmentor controller that realises the GUI logic behind the view.
  QScopedPointer<CaffeSegController> m_CaffeSegController;
};

} // end namespace

#endif
