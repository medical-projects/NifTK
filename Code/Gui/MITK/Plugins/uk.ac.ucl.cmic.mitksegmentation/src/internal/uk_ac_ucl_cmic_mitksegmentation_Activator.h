/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $LastChangedDate$
 Revision          : $Revision$
 Last modified by  : $Author$

 Original author   : $Author$

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/
 


#ifndef uk_ac_ucl_cmic_mitksegmentation_Activator_h
#define uk_ac_ucl_cmic_mitksegmentation_Activator_h

#include <ctkPluginActivator.h>

namespace mitk {

/**
 * \class uk_ac_ucl_cmic_mitksegmentation_Activator
 * \brief Class to launch/activate the MITKSegmentationView.
 * \ingroup uk_ac_ucl_cmic_mitksegmentation_internal
 */
class uk_ac_ucl_cmic_mitksegmentation_Activator :
  public QObject, public ctkPluginActivator
{
  Q_OBJECT
  Q_INTERFACES(ctkPluginActivator)

public:

  void start(ctkPluginContext* context);
  void stop(ctkPluginContext* context);

}; // uk_ac_ucl_cmic_mitksegmentation_Activator

}

#endif // uk_ac_ucl_cmic_mitksegmentation_Activator_h
