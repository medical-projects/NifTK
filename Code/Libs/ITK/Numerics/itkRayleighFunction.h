/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.
 
 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2012-10-17 15:10:23 +0100 (Wed, 17 Oct 2012) $
 Revision          : $Revision: 9581 $
 Last modified by  : $Author: jhh $

 Original author   : j.hipwell@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#ifndef ITK_RAYLEIGHFUNCTION_H
#define ITK_RAYLEIGHFUNCTION_H

#include "vnl/vnl_least_squares_function.h"


class ITK_EXPORT RayleighFunction : public vnl_least_squares_function
{
public:

RayleighFunction(unsigned int nDataPoints,
		 vnl_vector< double > &xData,
		 vnl_vector< double > &yData,
		 bool flgUseGradient ) 
    : vnl_least_squares_function(2, nDataPoints,
				 flgUseGradient ? use_gradient : no_gradient) 
  {
    m_nDataPoints = nDataPoints;

    m_xData = &xData;
    m_yData = &yData;
  }

  /// Compute the value of the Rayleigh distribution at a given index 'i'
  double compute(unsigned int i, double k, double s) 
  {
    return compute( (*m_xData)[i], k, s );
  }

  /// Compute the value of the Rayleigh distribution at a given 'x'
  double compute(double x, double k, double s) 
  {
    double sSq = s*s;
    return k*x*exp( -x*x/(2.*sSq) )/sSq;
  }

  /// Compute the derivative with respect to 'k' at index 'i'
  double compute_dydk(unsigned int i, double k, double s) 
  {
    return compute_dydk( (*m_xData)[i], k, s );
  }

  /// Compute the derivative with respect to 'k' at 'x'
  double compute_dydk(double x, double k, double s) 
  {
    double sSq = s*s;
    return x*exp( -x*x/(2.*sSq) )/sSq;
  }

  /// Compute the derivative with respect to 's' at index 'i'
  double compute_dydsig(unsigned int i, double k, double s) 
  {
    return compute_dydsig( (*m_xData)[i], k, s );
  }

  /// Compute the derivative with respect to 's' at 'x'
  double compute_dydsig(double x, double k, double s) 
  {
    double sSq = s*s;
    return x*(k*x*x - 2.*k*sSq)*exp( -x*x/(2.*sSq) )/(sSq*sSq*s);
  }

  /// Evaluate the vector of residual for parameters 'a'

  void f(const vnl_vector<double> &a, vnl_vector<double> &residual) 
  {
    for (unsigned int i=0; i<m_nDataPoints; ++i)       
    {
      residual[i] = compute(i, a[0], a[1] ) - (*m_yData)[i];
    }
  }

  /// Evaluate the gradient with respect to each parameter

  void gradf(const vnl_vector<double> &a, vnl_matrix<double> &J) 
  {
    for (unsigned int i=0; i<m_nDataPoints; ++i) {
      J(i,0) = compute_dydk(i, a[0], a[1] );
    }

    for (unsigned int i=0; i<m_nDataPoints; ++i) {
      J(i,1) = compute_dydsig(i, a[0], a[1] );
    }
  }

protected:

  /// The number of data points
  unsigned int m_nDataPoints;

  /// The 'x' ordinate data values
  vnl_vector< double > *m_xData;
  /// The 'y' ordinate data values
  vnl_vector< double > *m_yData;
  

};

#endif
