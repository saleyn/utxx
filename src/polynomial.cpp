//----------------------------------------------------------------------------
/// \file  polynomial.cpp
//----------------------------------------------------------------------------
/// \brief Polynomial curve fitting functions
//----------------------------------------------------------------------------
// Copyright (c) 2018 Serge Aleynikov <saleyn@gmail.com>
// Created: 2018-06-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2018 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/

#include <tuple>
#include <limits>
#include <cstdlib>
#include <utxx/polynomial.hpp>

namespace utxx {

std::tuple<double, double, double> quad_polynomial(double x[], double y[], size_t n)
{
  static constexpr const std::tuple<double, double, double>
    null = std::make_tuple(std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::quiet_NaN());
  if (n == 0)
    return null;

  double xm=0, ym=0, x2m=0, x3m=0, x4m=0, xym=0, x2ym=0;
  for (auto i=0u; i < n; ++i) {
    auto z  = x[i];
    auto z2 = z*z;
    xm     += z;
    ym     += y[i];
    x2m    += z*z;
    x3m    += z2*z;
    x4m    += z2*z2;
    xym    += z*y[i];
    x2ym   += z2*y[i];
  }
  xm   /= n;
  ym   /= n;
  x2m  /= n;
  x3m  /= n;
  x4m  /= n;
  xym  /= n;
  x2ym /= n;

  double sxx   = x2m  - xm  * xm;
  double sxy   = xym  - xm  * ym;
  double sxx2  = x3m  - xm  * x2m;
  double sx2x2 = x4m  - x2m * x2m;
  double sx2y  = x2ym - x2m * ym;

  double div   = sxx * sx2x2 - sxx2 * sxx2;

  if (div == 0.0)
    return null;

  double b = (sxy * sx2x2 - sx2y * sxx2) / div;
  double c = (sx2y *  sxx - sxy  * sxx2) / div;
  double a = ym - b * xm - c * x2m;

  return std::make_tuple(a, b, c);
}

} // namespace utxx

