/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkPointSetInteractor_h
#define niftkPointSetInteractor_h

#include "niftkMIDASExports.h"

#include <vector>

#include <mitkPointSetInteractor.h>

#include "niftkStateMachine.h"

namespace niftk
{
/**
 * \class PointSetInteractor
 * \brief Derived from mitk::PointSetInteractor so we can handle the mouse move event.
 * \ingroup Interaction
 */
class NIFTKMIDAS_EXPORT PointSetInteractor : public mitk::PointSetInteractor, public MIDASStateMachine
{
public:
  mitkClassMacro(PointSetInteractor, mitk::PointSetInteractor);
  mitkNewMacro3Param(Self, const char*, mitk::DataNode*, int);
  mitkNewMacro2Param(Self, const char*, mitk::DataNode*);

protected:
  /**
   * \brief Constructor with Param n for limited Set of Points
   *
   * If no n is set, then the number of points is unlimited
   * n=0 is not supported. In this case, n is set to 1.
   */
  PointSetInteractor(const char * type, mitk::DataNode* dataNode, int n = -1);

  /**
   * \brief Default Destructor
   **/
  virtual ~PointSetInteractor();

  /// \brief Tells if this tool can handle the given event.
  ///
  /// This implementation delegates the call to MIDASStateMachine::CanHandleEvent(),
  /// that checks if the event is filtered by one of the installed event filters and if not,
  /// calls CanHandle() and returns with its result.
  ///
  /// Note that this function is purposefully not virtual. Eventual subclasses should
  /// override the CanHandle function.
  float CanHandleEvent(const mitk::StateEvent* stateEvent) const;

  /**
   * \brief overriden the base class function, to enable mouse move events.
   */
  virtual float CanHandle(const mitk::StateEvent* stateEvent) const;

  /**
  * @brief Convert the given Actions to Operations and send to data and UndoController
  *
  * Overrides mitk::PointSetInteractor::ExecuteAction() so that for any operation the
  * display position is modified to be in the middle of a pixel.
  */
  virtual bool ExecuteAction( mitk::Action* action, mitk::StateEvent const* stateEvent );

};
}
#endif
