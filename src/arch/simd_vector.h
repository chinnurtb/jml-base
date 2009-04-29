/* simd_vector.h                                                   -*- C++ -*-
   Jeremy Barnes, 1 February 2005
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

   Generic SIMD vectorized loop kernels.
*/

#ifndef __arch__simd_vector_h__
#define __arch__simd_vector_h__

#include "simd.h"
#include "arch/arch.h"

namespace ML {

namespace SIMD {
namespace Generic {

/* Float versions */
void vec_scale(const float * x, float factor, float * r, size_t n);
void vec_add(const float * x, const float * y, float * r, size_t n);
void vec_add(const float * x, float k, const float * y, float * r, size_t n);
float vec_dotprod(const float * x, const float * y, size_t n);
void vec_minus(const float * x, const float * y, float * r, size_t n);
double vec_accum_prod3(const float * x, const float * y, const float * z,
                      size_t n);

/* Double versions */
void vec_scale(const double * x, double factor, double * r, size_t n);
void vec_add(const double * x, const double * y, double * r, size_t n);
void vec_add(const double * x, double k, const double * y, double * r,
             size_t n);
double vec_dotprod(const double * x, const double * y, size_t n);
void vec_minus(const double * x, const double * y, double * r, size_t n);
double vec_accum_prod3(const double * x, const double * y, const double * z,
                      size_t n);

/* Mixed float-double versions */
double vec_dotprod_dp(const float * x, const float * y, size_t n);
double vec_sum_dp(const float * x, size_t n);
inline double vec_dotprod_dp(const double * x, const double * y, size_t n)
{
    return vec_dotprod(x, y, n);
}
void vec_add(const double * x, double k, const float * y, double * r,
             size_t n);

} // namespace Generic

#if JML_USE_SSE1

namespace SSE1 {
} // namespace SSE1

namespace SSE2 {
} // namespace SSE2

namespace SSE3 {
} // namespace SSE3

#endif /* __i686 */

using namespace Generic;

} // namespace SIMD
} // namespace ML


#endif /* __arch__simd_vector_h__ */
