/* float_traits.h                                                  -*- C++ -*-
   Jeremy Barnes, 30 January 2005
   Copyright (c) 2005 Jeremy Barnes.  All rights reserved.
   
   This file is part of "Jeremy's Machine Learning Library", copyright (c)
   1999-2005 Jeremy Barnes.
   
   This program is available under the GNU General Public License, the terms
   of which are given by the file "license.txt" in the top level directory of
   the source code distribution.  If this file is missing, you have no right
   to use the program; please contact the author.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   ---
   
   Floating point traits.
*/

#ifndef __utils__float_traits_h__
#define __utils__float_traits_h__

namespace ML {

template<typename F1, typename F2>
struct float_traits {
    typedef typeof(F1() + F2()) return_type;
};

template <typename F>
struct float_traits<F, F> {
    typedef F return_type;
};

template<typename F1, typename F2, typename F3>
struct float_traits3 {
    typedef typeof(*((F1*)(0)) + (*((F2*)(0))) + (*((F3*)(0)))) return_type;
};

template <typename F>
struct float_traits3<F, F, F> {
    typedef F return_type;
};

#if 0

#define FLOAT_SPECIALIZATION(f1, f2, result) \
template<> struct float_traits<f1, f2> { \
    typedef result return_type; \
};

template<> struct float_traits<f2, f1> { \
    typedef result return_type; \
};

FLOAT_SPECIALIZATION(


template<>
struct float_traits<int, int> {

#endif

} // namespace ML

#endif /* __utils__float_traits_h__ */
