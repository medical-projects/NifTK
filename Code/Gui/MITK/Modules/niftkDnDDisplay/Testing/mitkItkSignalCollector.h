/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef __mitkItkSignalCollector_h
#define __mitkItkSignalCollector_h

#include <itkCommand.h>
#include <itkEventObject.h>

#include <mitkCommon.h>

#include <vector>

namespace mitk
{

class ItkSignalCollector : public itk::Command
{
public:
  mitkClassMacro(ItkSignalCollector, itk::Command);
  itkNewMacro(ItkSignalCollector);

  typedef std::pair<const itk::Object*, itk::EventObject*> Signal;
  typedef std::vector<Signal> Signals;

  void Connect(itk::Object* object, const itk::EventObject& event);

  /// \brief Gets the signals collected by this object.
  const Signals& GetSignals() const;

  /// \brief Clears all the signals collected by now.
  virtual void Clear();

  /// \brief Called when the event happens to the caller.
  virtual void ProcessEvent(const itk::Object* object, const itk::EventObject& event);

protected:

  /// \brief Constructs an ItkSignalCollector object.
  ItkSignalCollector();

  /// \brief Destructs an ItkSignalCollector object.
  virtual ~ItkSignalCollector();

  /// \brief Prints the collected signals to the given stream or to the standard output if no stream is given.
  virtual void PrintSelf(std::ostream & os, itk::Indent indent) const;

private:

  /// \brief Called when the event happens to the caller.
  virtual void Execute(itk::Object* caller, const itk::EventObject& event);

  /// \brief Called when the event happens to the caller.
  virtual void Execute(const itk::Object* object, const itk::EventObject& event);

  typedef std::multimap<itk::Object::Pointer, unsigned long> ObserverMap;
  ObserverMap m_ObserverTags;

  /// \brief The signals collected by this object.
  Signals m_Signals;
};

}

#endif
