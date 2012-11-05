/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2011-11-18 09:05:48 +0000 (Fri, 18 Nov 2011) $
 Revision          : $Revision: 7804 $
 Last modified by  : $Author: mjc $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#ifndef QMITKNIFTYVIEWAPPLICATIONPREFERENCEPAGE_H_
#define QMITKNIFTYVIEWAPPLICATIONPREFERENCEPAGE_H_

#include "berryIQtPreferencePage.h"
#include <berryIPreferences.h>

class QWidget;
class QRadioButton;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;

/**
 * \class QmitkNiftyViewApplicationPreferencePage
 * \brief Preferences page for this plugin, providing application wide defaults.
 * \ingroup uk_ac_ucl_cmic_gui_qt_niftyview_internal
 *
 */
class QmitkNiftyViewApplicationPreferencePage : public QObject, public berry::IQtPreferencePage
{
  Q_OBJECT
  Q_INTERFACES(berry::IPreferencePage)

public:

  QmitkNiftyViewApplicationPreferencePage();
  QmitkNiftyViewApplicationPreferencePage(const QmitkNiftyViewApplicationPreferencePage& other);
  ~QmitkNiftyViewApplicationPreferencePage();

  static const std::string IMAGE_INITIALISATION_METHOD_NAME;
  static const std::string IMAGE_INITIALISATION_MIDAS;
  static const std::string IMAGE_INITIALISATION_LEVELWINDOW;
  static const std::string IMAGE_INITIALISATION_PERCENTAGE;
  static const std::string IMAGE_INITIALISATION_PERCENTAGE_NAME;
  static const std::string IMAGE_RESLICE_INTERPOLATION;
  static const std::string IMAGE_TEXTURE_INTERPOLATION;

  void Init(berry::IWorkbench::Pointer workbench);

  void CreateQtControl(QWidget* widget);

  QWidget* GetQtControl() const;

  ///
  /// \see IPreferencePage::PerformOk()
  ///
  virtual bool PerformOk();

  ///
  /// \see IPreferencePage::PerformCancel()
  ///
  virtual void PerformCancel();

  ///
  /// \see IPreferencePage::Update()
  ///
  virtual void Update();

protected slots:

  void OnMIDASInitialisationRadioButtonChecked(bool);
  void OnLevelWindowRadioButtonChecked(bool);
  void OnImageDataRadioButtonChecked(bool);

protected:

  QWidget        *m_MainControl;
  QRadioButton   *m_UseMidasInitialisationRadioButton;
  QRadioButton   *m_UseLevelWindowRadioButton;
  QRadioButton   *m_UseImageDataRadioButton;
  QDoubleSpinBox *m_PercentageOfDataRangeDoubleSpinBox;
  QComboBox      *m_ResliceInterpolationComboBox;
  QComboBox      *m_TextureInterpolationComboBox;

  bool m_Initializing;

  berry::IPreferences::Pointer m_PreferencesNode;
};

#endif /* QMITKNIFTYVIEWAPPLICATIONPREFERENCEPAGE_H_ */

