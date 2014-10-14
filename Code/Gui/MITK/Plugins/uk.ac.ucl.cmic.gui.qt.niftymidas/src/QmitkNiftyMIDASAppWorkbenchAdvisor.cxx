/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "QmitkNiftyMIDASAppWorkbenchAdvisor.h"
#include "QmitkNiftyMIDASWorkbenchWindowAdvisor.h"

#include <berryIWorkbenchConfigurer.h>
#include <QmitkMultiViewerEditor.h>
#include <niftkMultiViewerWidget.h>

#include <mitkLogMacros.h>
#include <mitkIDataStorageReference.h>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QApplication>

#include <QmitkCommonAppsApplicationPlugin.h>

//-----------------------------------------------------------------------------
std::string QmitkNiftyMIDASAppWorkbenchAdvisor::GetInitialWindowPerspectiveId()
{
  return "uk.ac.ucl.cmic.gui.qt.niftymidas.segmentation_perspective";
}


//-----------------------------------------------------------------------------
std::string QmitkNiftyMIDASAppWorkbenchAdvisor::GetWindowIconResourcePath() const
{
  return ":/QmitkNiftyMIDASApplication/icon_ion.xpm";
}


//-----------------------------------------------------------------------------
QmitkBaseWorkbenchWindowAdvisor* QmitkNiftyMIDASAppWorkbenchAdvisor::CreateQmitkBaseWorkbenchWindowAdvisor(
    berry::IWorkbenchWindowConfigurer::Pointer configurer)
{
  return new QmitkNiftyMIDASWorkbenchWindowAdvisor(this, configurer);
}


//-----------------------------------------------------------------------------
void QmitkNiftyMIDASAppWorkbenchAdvisor::PostStartup()
{
  std::vector<std::string> args = berry::Platform::GetApplicationArgs();

  berry::IWorkbenchConfigurer::Pointer workbenchConfigurer = this->GetWorkbenchConfigurer();
  berry::IWorkbench* workbench = workbenchConfigurer->GetWorkbench();
  berry::IWorkbenchWindow::Pointer workbenchWindow = workbench->GetActiveWorkbenchWindow();
  if (!workbenchWindow)
  {
    std::vector<berry::IWorkbenchWindow::Pointer> workbenchWindows = workbench->GetWorkbenchWindows();
    if (!workbenchWindows.empty())
    {
      workbenchWindow = workbenchWindows[0];
    }
    else
    {
      /// TODO there is no active workbench window.
      MITK_INFO << "There is no active workbench window.";
    }
  }

  int viewerRows = 0;
  int viewerColumns = 0;

  for (std::vector<std::string>::const_iterator it = args.begin(); it != args.end(); ++it)
  {
    std::string arg = *it;
    if (arg == "--perspective")
    {
      ++it;
      if (it == args.end())
      {
        /// TODO invalid argument
        MITK_INFO << "Missing argument for perspective.";
        break;
      }

      std::string perspectiveLabel = *it;

      berry::IPerspectiveRegistry* perspectiveRegistry = workbench->GetPerspectiveRegistry();
      berry::IPerspectiveDescriptor::Pointer perspectiveDescriptor = perspectiveRegistry->FindPerspectiveWithLabel(perspectiveLabel);

      if (perspectiveDescriptor.IsNull())
      {
        MITK_ERROR << "Unknown perspective.";
        continue;
      }

      workbench->ShowPerspective(perspectiveDescriptor->GetId(), workbenchWindow);
    }
    else if (arg == "--window-layout")
    {
      ++it;
      if (it == args.end())
      {
        /// TODO invalid argument
        MITK_INFO << "Missing argument for window layout.";
        break;
      }

      std::string windowLayoutName = *it;

      WindowLayout windowLayout = ::GetWindowLayout(windowLayoutName);

      if (windowLayout != WINDOW_LAYOUT_UNKNOWN)
      {
        berry::IEditorPart::Pointer activeEditor = workbenchWindow->GetActivePage()->GetActiveEditor();
        QmitkMultiViewerEditor* dndDisplay = dynamic_cast<QmitkMultiViewerEditor*>(activeEditor.GetPointer());
        niftkMultiViewerWidget* multiViewer = dndDisplay->GetMultiViewer();
        multiViewer->SetDefaultWindowLayout(windowLayout);
      }
    }
    else if (arg == "--dnd" || arg == "--drag-and-drop")
    {
      ++it;
      if (it == args.end())
      {
        break;
      }

      QString nodeNamesArg = QString::fromStdString(*it);

      QStringList nodeNames = nodeNamesArg.split(",");

      mitk::DataStorage::Pointer dataStorage = this->GetDataStorage();

      std::vector<mitk::DataNode*> nodes;

      foreach (QString nodeName, nodeNames)
      {
        mitk::DataNode* node = dataStorage->GetNamedNode(nodeName.toStdString());
        if (node)
        {
          nodes.push_back(node);
        }
      }

      berry::IEditorPart::Pointer activeEditor = workbenchWindow->GetActivePage()->GetActiveEditor();
      QmitkMultiViewerEditor* dndDisplay = dynamic_cast<QmitkMultiViewerEditor*>(activeEditor.GetPointer());
      niftkMultiViewerWidget* multiViewer = dndDisplay->GetMultiViewer();

      niftkSingleViewerWidget* viewer = multiViewer->GetSelectedViewer();

      QmitkRenderWindow* selectedWindow = viewer->GetSelectedRenderWindow();

      this->DropNodes(selectedWindow, nodes);
    }
    else if (arg == "--viewer-rows")
    {
      ++it;
      if (it == args.end())
      {
        /// TODO invalid argument
        MITK_INFO << "Missing argument for viewer rows.";
        break;
      }

      QString viewerRowsArg = QString::fromStdString(*it);

      bool ok = true;
      viewerRows = viewerRowsArg.toInt(&ok);
    }
    else if (arg == "--viewer-columns")
    {
      ++it;
      if (it == args.end())
      {
        /// TODO invalid argument
        MITK_INFO << "Missing argument for viewer columns.";
        break;
      }

      QString viewerColumnsArg = QString::fromStdString(*it);

      bool ok = true;
      viewerColumns = viewerColumnsArg.toInt(&ok);
    }
    else if (arg == "--bind-viewers")
    {
      ++it;

      if (it == args.end())
      {
        /// TODO invalid argument
        MITK_INFO << "Missing argument for viewer bindings.";
        break;
      }

      QString viewerBindingArg = QString::fromStdString(*it);

      QStringList viewerBindingOptions = viewerBindingArg.split(",");

      berry::IEditorPart::Pointer activeEditor = workbenchWindow->GetActivePage()->GetActiveEditor();
      QmitkMultiViewerEditor* dndDisplay = dynamic_cast<QmitkMultiViewerEditor*>(activeEditor.GetPointer());
      niftkMultiViewerWidget* multiViewer = dndDisplay->GetMultiViewer();

      int bindingOptions = 0;

      foreach (QString viewerBindingOption, viewerBindingOptions)
      {
        bool value;

        QStringList viewerBindingOptionParts = viewerBindingOption.split("=");
        if (viewerBindingOptionParts.size() != 1 && viewerBindingOptionParts.size() != 2)
        {
          /// TODO invalid argument
          MITK_INFO << "Invalid argument format for viewer bindings.";
          continue;
        }

        QString viewerBindingOptionName = viewerBindingOptionParts[0];

        if (viewerBindingOptionParts.size() == 1)
        {
          value = true;
        }
        else if (viewerBindingOptionParts.size() == 2)
        {
          QString viewerBindingOptionValue = viewerBindingOptionParts[1];

          if (viewerBindingOptionValue == QString("1")
              || viewerBindingOptionValue == QString("true")
              || viewerBindingOptionValue == QString("on")
              || viewerBindingOptionValue == QString("yes")
              )
          {
            value = true;
          }
          else if (viewerBindingOptionValue == QString("0")
              || viewerBindingOptionValue == QString("false")
              || viewerBindingOptionValue == QString("off")
              || viewerBindingOptionValue == QString("no")
              )
          {
            value = false;
          }
          else
          {
            /// TODO invalid argument
            MITK_INFO << "Invalid argument format for viewer bindings.";
            continue;
          }
        }
        else
        {
          /// TODO invalid argument
          MITK_INFO << "Invalid argument format for viewer bindings.";
          continue;
        }


        if (viewerBindingOptionName == QString("position"))
        {
          if (value)
          {
            bindingOptions |= niftkMultiViewerWidget::PositionBinding;
          }
          else
          {
            bindingOptions &= ~niftkMultiViewerWidget::PositionBinding;
          }
        }
        else if (viewerBindingOptionName == QString("cursor"))
        {
          if (value)
          {
            bindingOptions |= niftkMultiViewerWidget::CursorBinding;
          }
          else
          {
            bindingOptions &= ~niftkMultiViewerWidget::CursorBinding;
          }
        }
        else if (viewerBindingOptionName == QString("magnification"))
        {
          if (value)
          {
            bindingOptions |= niftkMultiViewerWidget::MagnificationBinding;
          }
          else
          {
            bindingOptions &= ~niftkMultiViewerWidget::MagnificationBinding;
          }
        }
        else if (viewerBindingOptionName == QString("layout"))
        {
          if (value)
          {
            bindingOptions |= niftkMultiViewerWidget::WindowLayoutBinding;
          }
          else
          {
            bindingOptions &= ~niftkMultiViewerWidget::WindowLayoutBinding;
          }
        }
        else if (viewerBindingOptionName == QString("geometry"))
        {
          if (value)
          {
            bindingOptions |= niftkMultiViewerWidget::GeometryBinding;
          }
          else
          {
            bindingOptions &= ~niftkMultiViewerWidget::GeometryBinding;
          }
        }
        else if (viewerBindingOptionName == QString("all"))
        {
          if (value)
          {
            bindingOptions =
                niftkMultiViewerWidget::PositionBinding
                | niftkMultiViewerWidget::CursorBinding
                | niftkMultiViewerWidget::MagnificationBinding
                | niftkMultiViewerWidget::WindowLayoutBinding
                | niftkMultiViewerWidget::GeometryBinding
                ;
          }
          else
          {
            bindingOptions = 0;
          }
        }
        else if (viewerBindingOptionName == QString("none"))
        {
          bindingOptions = 0;
        }
        else
        {
          /// TODO invalid argument
          continue;
        }
      }

      multiViewer->SetBindingOptions(bindingOptions);
    }
  }

  if (viewerRows != 0 || viewerColumns != 0)
  {
    if (viewerRows == 0)
    {
      viewerRows = 1;
    }
    if (viewerColumns == 0)
    {
      viewerColumns = 1;
    }

    berry::IEditorPart::Pointer activeEditor = workbenchWindow->GetActivePage()->GetActiveEditor();
    QmitkMultiViewerEditor* dndDisplay = dynamic_cast<QmitkMultiViewerEditor*>(activeEditor.GetPointer());
    niftkMultiViewerWidget* multiViewer = dndDisplay->GetMultiViewer();
    multiViewer->SetViewerNumber(viewerRows, viewerColumns);
  }
}


// --------------------------------------------------------------------------
mitk::DataStorage* QmitkNiftyMIDASAppWorkbenchAdvisor::GetDataStorage()
{
  mitk::DataStorage::Pointer dataStorage = 0;

  ctkPluginContext* context = QmitkCommonAppsApplicationPlugin::GetDefault()->GetPluginContext();
  ctkServiceReference dsServiceRef = context->getServiceReference<mitk::IDataStorageService>();
  if (dsServiceRef)
  {
    mitk::IDataStorageService* dsService = context->getService<mitk::IDataStorageService>(dsServiceRef);
    if (dsService)
    {
      mitk::IDataStorageReference::Pointer dataStorageRef = dsService->GetActiveDataStorage();
      dataStorage = dataStorageRef->GetDataStorage();
    }
  }

  return dataStorage;
}


// --------------------------------------------------------------------------
void QmitkNiftyMIDASAppWorkbenchAdvisor::DropNodes(QmitkRenderWindow* renderWindow, const std::vector<mitk::DataNode*>& nodes)
{
  QMimeData* mimeData = new QMimeData;
  QString dataNodeAddresses("");
  for (int i = 0; i < nodes.size(); ++i)
  {
    long dataNodeAddress = reinterpret_cast<long>(nodes[i]);
    QTextStream(&dataNodeAddresses) << dataNodeAddress;

    if (i != nodes.size() - 1)
    {
      QTextStream(&dataNodeAddresses) << ",";
    }
  }
  mimeData->setData("application/x-mitk-datanodes", QByteArray(dataNodeAddresses.toAscii()));
//  QStringList types;
//  types << "application/x-mitk-datanodes";
  QDragEnterEvent dragEnterEvent(renderWindow->rect().center(), Qt::CopyAction | Qt::MoveAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  QDropEvent dropEvent(renderWindow->rect().center(), Qt::CopyAction | Qt::MoveAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  dropEvent.acceptProposedAction();
  if (!qApp->notify(renderWindow, &dragEnterEvent))
  {
    MITK_WARN << "Drag enter event not accepted by receiving widget.";
  }
  if (!qApp->notify(renderWindow, &dropEvent))
  {
    MITK_WARN << "Drop event not accepted by receiving widget.";
  }
}
