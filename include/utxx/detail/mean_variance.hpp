//----------------------------------------------------------------------------
/// \file mean_variance.hpp
//----------------------------------------------------------------------------
/// This file contains classes used for calculating mean and
/// variance.  They are used mainly in unit tests.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@mail.com>
// Created: 2010-05-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_MEAN_VARIANCE_HPP_
#define _UTXX_MEAN_VARIANCE_HPP_

namespace utxx {
namespace detail {

template <class T>
double mean(const T* begin, const T* end) {
    size_t count = 0;
    double sum = 0;
    for (const T* p=begin; p != end; ++p, ++count)
        sum += *p;
    return (double)sum / count;
}

template <class T>
double variance(const T* begin, const T* end) {
    double avg = mean(begin, end);
    size_t count = 0;
    double sum = 0;
    for (const T* p=begin; p != end; ++p, ++count)
        sum += (*p - avg) * (*p - avg);
    return sum / count;
}

// A numerically stable algorithm for the computation 
// of the variance (and the mean as well).
template <class T>
double online_variance(const T* begin, const T* end) {
    double count = 0;
    double mean = 0;
    double sum = 0;

    for (const T* p=begin; p != end; ++p) {
        count = count + 1;
        double delta = *p - mean;
        mean = mean + delta/count;
        sum = sum + delta*(*p - mean);
    }

    return sum/count;
}

}  // namespace detail
}  // namespace utxx

#endif // _UTXX_MEAN_VARIANCE_HPP_

