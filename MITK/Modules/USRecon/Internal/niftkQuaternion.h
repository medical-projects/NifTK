/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkQuaternion_h
#define niftkQuaternion_h

#include <iostream>  // C++ I/O
#include <iomanip>   // setprecision(),...
#include <cassert>    // assert()
#include <vector>

using std::vector;

class niftkQuaternion : public vector<double>
{
	double m_Angle, m_Axis[3];

public:
	// constructors
  niftkQuaternion() : vector<double>(4, 0)
  {
    m_Angle = 0;
    m_Axis[0] = m_Axis[1] = m_Axis[2] = 0;
  }

  niftkQuaternion(const vector<double>& v) : vector<double>(4)
	{
		int s = (int)v.size(); 
    
    if(s>4) s = 4;
		
    for(int i=0; i<s; i++) begin()[i] = v[i];

     m_Angle = 0;
     m_Axis[0] = m_Axis[1] = m_Axis[2] = 0;
	}

  niftkQuaternion(double angle, const vector<double> v); // Create the quaternion from a rotation around an aixs
	niftkQuaternion(double A, double B, double C, int flag); // Create the quaternion from three Euler angles 

  void		Create(double a, double b, double c, double d);
	void		CreateFromEulerAngles(double a, double b, double c, int flag);

	niftkQuaternion& operator = (const vector<double>& v)
	{
		clear();
		int s = (int)v.size(); if(s>4) s = 4;
		for(int i=0; i<s; i++) begin()[i] = v[i];

    return *this;
  }

	// member functions
	bool		IsUnit() const;
	void		Resize(int) const { return; }
	double		Norm() const;
	
	// overloaded operators
	niftkQuaternion		Conjugate() const ;
	void			GetAngleAxis(double& angle, double* axis);
	void			SetAngleAxis(double angle, double x, double y, double z);

	niftkQuaternion		operator - (const niftkQuaternion&) const;
	niftkQuaternion		operator + (const niftkQuaternion&) const;
	niftkQuaternion		operator * (const niftkQuaternion&) const;
	niftkQuaternion&	operator *= (const niftkQuaternion&);
	double&			operator [] (int index);
	const double&	operator [] (int index) const;
}; // class niftkQuaternion


inline niftkQuaternion operator + (const niftkQuaternion& v) { return v; }
inline niftkQuaternion operator - (const niftkQuaternion& v) { niftkQuaternion r; r.Create(-v[0], -v[1], -v[2], -v[3]); return r; }


inline double& niftkQuaternion::operator [] (int index)
{
	assert (index >= 0 && index < 4) ;
	return begin()[index];
}


// subscript operator [] (const object)
inline const double& niftkQuaternion::operator [] (int index) const
{
	assert (index >= 0 && index < 4) ;
	return begin()[index];
}


// overloaded insertion operator <<
inline std::ostream& operator << (std::ostream& s, const niftkQuaternion& v)
{
	s << "[" ;
	for (int i=0; i<4; i++)
	{
		s << std::setprecision (2)
		  << std::setiosflags (std::ios::showpoint | std::ios::fixed)
		  << v[i] ;
		i!=3 ? s << ", " : s << "]" ;
	}
	return s ;
}

#endif // niftkQuaternion_h
