/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef IGIVideoOverlayEditor_h
#define IGIVideoOverlayEditor_h

#include <QmitkAbstractRenderEditor.h>
#include <mitkILinkedRenderWindowPart.h>
#include <service/event/ctkEvent.h>

namespace niftk
{

class IGIVideoOverlayEditorPrivate;
class IGIVideoOverlayWidget;

/**
 * \class IGIVideoOverlayEditor
 * \brief Simple editor that delegates all functionality to a niftk::IGIVideoOverlayWidget,
 * and most methods are dummy or do-nothing implementations, as the widget is for
 * a very specific purpose and most of the mitk::ILinkedRenderWindowPart are not needed.
 * \ingroup uk_ac_ucl_cmic_igivideooverlayeditor_internal
 */
class IGIVideoOverlayEditor
  : public QmitkAbstractRenderEditor, public mitk::ILinkedRenderWindowPart
{
  Q_OBJECT

public:

  static const QString EDITOR_ID;

  berryObjectMacro(IGIVideoOverlayEditor)

  IGIVideoOverlayEditor();
  ~IGIVideoOverlayEditor();

  niftk::IGIVideoOverlayWidget* GetIGIVideoOverlayWidget();

  // -------------------  mitk::IRenderWindowPart  ----------------------

  /**
   * \see mitk::IRenderWindowPart::GetActiveQmitkRenderWindow()
   */
  QmitkRenderWindow* GetActiveQmitkRenderWindow() const override;

  /**
   * \see mitk::IRenderWindowPart::GetQmitkRenderWindows()
   */
  QHash<QString,QmitkRenderWindow*> GetQmitkRenderWindows() const override;

  /**
   * \see mitk::IRenderWindowPart::GetQmitkRenderWindow(QString)
   */
  QmitkRenderWindow* GetQmitkRenderWindow(const QString& id) const override;

  /**
   * \see mitk::IRenderWindowPart::GetSelectionPosition()
   */
  mitk::Point3D GetSelectedPosition(const QString& id = QString()) const override;

  /**
   * \see mitk::IRenderWindowPart::SetSelectedPosition()
   */
  void SetSelectedPosition(const mitk::Point3D& pos, const QString& id = QString()) override;

  /**
   * \see mitk::IRenderWindowPart::EnableDecorations()
   */
  void EnableDecorations(bool enable, const QStringList& decorations = QStringList()) override;

  /**
   * \see mitk::IRenderWindowPart::IsDecorationEnabled()
   */
  bool IsDecorationEnabled(const QString& decoration) const override;

  /**
   * \see mitk::IRenderWindowPart::GetDecorations()
   */
  QStringList GetDecorations() const override;

  // -------------------  mitk::ILinkedRenderWindowPart  ----------------------

  /**
   * \see mitk::ILinkedRenderWindowPart::GetSlicesRotator()
   */
  mitk::SlicesRotator* GetSlicesRotator() const override;

  /**
   * \see mitk::ILinkedRenderWindowPart::GetSlicesSwiveller()
   */
  mitk::SlicesSwiveller* GetSlicesSwiveller() const override;

  /**
   * \see mitk::ILinkedRenderWindowPart::EnableSlicingPlanes()
   */
  void EnableSlicingPlanes(bool enable) override;

  /**
   * \see mitk::ILinkedRenderWindowPart::IsSlicingPlanesEnabled()
   */
  bool IsSlicingPlanesEnabled() const override;

  /**
   * \see mitk::ILinkedRenderWindowPart::EnableLinkedNavigation()
   */
  void EnableLinkedNavigation(bool enable) override;

  /**
   * \see mitk::ILinkedRenderWindowPart::IsLinkedNavigationEnabled()
   */
  bool IsLinkedNavigationEnabled() const override;

protected:

  void SetFocus() override;
  void OnPreferencesChanged(const berry::IBerryPreferences*) override;
  void CreateQtPartControl(QWidget* parent) override;

protected slots:

  void OnPreferencesChanged();

private slots:

  /**
   * \brief We listen to "uk/ac/ucl/cmic/IGIUPDATE" and call this method.
   */
  void OnIGIUpdate(const ctkEvent& event);

private:

  const QScopedPointer<IGIVideoOverlayEditorPrivate> d;

};

} // end namespace

#endif
