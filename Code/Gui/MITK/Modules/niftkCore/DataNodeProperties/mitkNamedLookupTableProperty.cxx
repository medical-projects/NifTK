/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "mitkNamedLookupTableProperty.h"

namespace mitk {

//-----------------------------------------------------------------------------
NamedLookupTableProperty::NamedLookupTableProperty()
: Superclass()
, m_Name("n/a")
{
}


//-----------------------------------------------------------------------------
NamedLookupTableProperty::NamedLookupTableProperty(const NamedLookupTableProperty& other)
: Superclass(other)
, m_Name(other.m_Name)
{

}


//-----------------------------------------------------------------------------
NamedLookupTableProperty::NamedLookupTableProperty(const std::string& name, const mitk::LookupTable::Pointer lut)
: Superclass(lut)
, m_Name(name)
{
}


//-----------------------------------------------------------------------------
NamedLookupTableProperty::~NamedLookupTableProperty()
{
}


//-----------------------------------------------------------------------------
std::string NamedLookupTableProperty::GetValueAsString() const
{
  return m_Name;
}


//-----------------------------------------------------------------------------
itk::LightObject::Pointer NamedLookupTableProperty::InternalClone() const
{
  itk::LightObject::Pointer result(new Self(*this));
  return result;
}


//-----------------------------------------------------------------------------
bool NamedLookupTableProperty::IsEqual(const BaseProperty& property) const
{
  return *(this->m_LookupTable) == *(static_cast<const Self&>(property).m_LookupTable)
      && this->m_Name == static_cast<const Self&>(property).m_Name
      ;
}


//-----------------------------------------------------------------------------
bool NamedLookupTableProperty::Assign(const BaseProperty& property)
{
  this->m_LookupTable = static_cast<const Self&>(property).m_LookupTable;
  this->m_Name = static_cast<const Self&>(property).m_Name;
  return true;

}

} // namespace mitk