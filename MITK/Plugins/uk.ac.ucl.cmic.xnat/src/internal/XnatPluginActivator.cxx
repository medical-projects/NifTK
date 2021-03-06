/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "XnatPluginActivator.h"

#include "XnatBrowserView.h"
#include "XnatPluginPreferencePage.h"

namespace mitk {

void XnatPluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(XnatBrowserView, context);
  BERRY_REGISTER_EXTENSION_CLASS(XnatPluginPreferencePage, context);
}

void XnatPluginActivator::stop(ctkPluginContext* context)
{
  Q_UNUSED(context)
}

}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  Q_EXPORT_PLUGIN2(uk_ac_ucl_cmic_xnat, mitk::XnatPluginActivator)
#endif
