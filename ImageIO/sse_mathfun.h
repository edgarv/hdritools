/* SIMD (SSE1+MMX indeed) implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library
*/

/* Copyright (C) 2007  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

/*
  2008 - Edgar Velazquez-Armendariz: Converted to SSE2, removing MMX code.
  Retrieved from http://gruntthepeon.free.fr/ssemath/
*/

#include <xmmintrin.h>
#include <emmintrin.h>

/* yes I know, the top of this file is quite ugly */

#if !defined(ALIGN16_BEG) && !defined(ALIGN16_BEG)
#ifdef _MSC_VER /* visual c++ */
# define ALIGN16_BEG __declspec(align(16))
# define ALIGN16_END 
#else /* gcc or icc */
# define ALIGN16_BEG
# define ALIGN16_END __attribute__((aligned(16)))
#endif
#endif /* !defined(ALIGN16_BEG) && !defined(ALIGN16_BEG) */

/* __m128 is ugly to write */
typedef __m128  v4sf;
typedef __m128i v4si;
#if 0
typedef __m64 v2si;
#endif

/* declare some SSE constants -- why can't I figure a better way to do that? */
#define _PS_CONST(Name, Val)                                            \
  static const ALIGN16_BEG float _ps_##Name[4] ALIGN16_END = { Val, Val, Val, Val }
#define _PI32_CONST(Name, Val)                                            \
  static const ALIGN16_BEG int _pi32_##Name[4] ALIGN16_END = { Val, Val, Val, Val }
#define _PS_CONST_TYPE(Name, Type, Val)                                 \
  static const ALIGN16_BEG Type _ps_##Name[4] ALIGN16_END = { Val, Val, Val, Val }

_PS_CONST(1  , 1.0f);
_PS_CONST(0p5, 0.5f);
/* the smallest non denormalized float number */
_PS_CONST_TYPE(min_norm_pos, int, 0x00800000);
_PS_CONST_TYPE(mant_mask, int, 0x7f800000);
_PS_CONST_TYPE(inv_mant_mask, int, ~0x7f800000);

_PS_CONST_TYPE(sign_mask, unsigned int, 0x80000000);
_PS_CONST_TYPE(inv_sign_mask, int, ~0x80000000);

_PI32_CONST(1, 1);
_PI32_CONST(inv1, ~1);
_PI32_CONST(2, 2);
_PI32_CONST(4, 4);
_PI32_CONST(0x7f, 0x7f);

_PS_CONST(cephes_SQRTHF, 0.707106781186547524f);
_PS_CONST(cephes_log_p0, 7.0376836292E-2f);
_PS_CONST(cephes_log_p1, - 1.1514610310E-1f);
_PS_CONST(cephes_log_p2, 1.1676998740E-1f);
_PS_CONST(cephes_log_p3, - 1.2420140846E-1f);
_PS_CONST(cephes_log_p4, + 1.4249322787E-1f);
_PS_CONST(cephes_log_p5, - 1.6668057665E-1f);
_PS_CONST(cephes_log_p6, + 2.0000714765E-1f);
_PS_CONST(cephes_log_p7, - 2.4999993993E-1f);
_PS_CONST(cephes_log_p8, + 3.3333331174E-1f);
_PS_CONST(cephes_log_q1, -2.12194440e-4f);
_PS_CONST(cephes_log_q2, 0.693359375f);

    

/* natural logarithm computed for 4 simultaneous float 
   return NaN for x <= 0
*/
inline v4sf log_ps(v4sf x) {
  v4si mm;
  v4sf one = *(v4sf*)_ps_1;

  v4sf invalid_mask = _mm_cmple_ps(x, _mm_setzero_ps());

  x = _mm_max_ps(x, *(v4sf*)_ps_min_norm_pos);  /* cut off denormalized stuff */

  
  /* part 1: x = frexpf(x, &e); */
  mm = _mm_srli_epi32(*(v4si*)&x, 23);
  /* keep only the fractional part */
  x = _mm_and_ps(x, *(v4sf*)_ps_inv_mant_mask);
  x = _mm_or_ps(x, *(v4sf*)_ps_0p5);

  /* now e=mm0:mm1 contain the really base-2 exponent */
  // FIXME What is faster: loading from memory or setting them?
  mm = _mm_sub_epi32(mm, _mm_set1_epi32(0x7f));
  
  v4sf e = _mm_cvtepi32_ps(mm);  
  e = _mm_add_ps(e, one);

  /* part2: 
     if( x < SQRTHF ) {
       e -= 1;
       x = x + x - 1.0;
     } else { x = x - 1.0; }
  */
  v4sf mask = _mm_cmplt_ps(x, *(v4sf*)_ps_cephes_SQRTHF);
  //printf("log_ps: mask=");print2i(mmask.mm[0]); print2i(mmask.mm[1]); printf("\n");

  v4sf tmp = _mm_and_ps(x, mask);
  x = _mm_sub_ps(x, one);
  e = _mm_sub_ps(e, _mm_and_ps(one, mask));
  x = _mm_add_ps(x, tmp);


  v4sf z = _mm_mul_ps(x,x);

  v4sf y = *(v4sf*)_ps_cephes_log_p0;
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p1);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p2);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p3);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p4);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p5);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p6);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p7);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_log_p8);
  y = _mm_mul_ps(y, x);

  y = _mm_mul_ps(y, z);
  

  tmp = _mm_mul_ps(e, *(v4sf*)_ps_cephes_log_q1);
  y = _mm_add_ps(y, tmp);


  tmp = _mm_mul_ps(z, *(v4sf*)_ps_0p5);
  y = _mm_sub_ps(y, tmp);

  tmp = _mm_mul_ps(e, *(v4sf*)_ps_cephes_log_q2);
  x = _mm_add_ps(x, y);
  x = _mm_add_ps(x, tmp);
  x = _mm_or_ps(x, invalid_mask); // negative arg will be NAN

  return x;
}

_PS_CONST(exp_hi,	88.3762626647949f);
_PS_CONST(exp_lo,	-88.3762626647949f);

_PS_CONST(cephes_LOG2EF, 1.44269504088896341f);
_PS_CONST(cephes_exp_C1, 0.693359375f);
_PS_CONST(cephes_exp_C2, -2.12194440e-4f);

_PS_CONST(cephes_exp_p0, 1.9875691500E-4f);
_PS_CONST(cephes_exp_p1, 1.3981999507E-3f);
_PS_CONST(cephes_exp_p2, 8.3334519073E-3f);
_PS_CONST(cephes_exp_p3, 4.1665795894E-2f);
_PS_CONST(cephes_exp_p4, 1.6666665459E-1f);
_PS_CONST(cephes_exp_p5, 5.0000001201E-1f);

inline v4sf exp_ps(v4sf x) {
  v4sf tmp = _mm_setzero_ps(), fx;
  v4si mm;
  v4sf one = *(v4sf*)_ps_1;

  x = _mm_min_ps(x, *(v4sf*)_ps_exp_hi);
  x = _mm_max_ps(x, *(v4sf*)_ps_exp_lo);

  /* express exp(x) as exp(g + n*log(2)) */
  fx = _mm_mul_ps(x, *(v4sf*)_ps_cephes_LOG2EF);
  fx = _mm_add_ps(fx, *(v4sf*)_ps_0p5);

  /* how to perform a floorf with SSE: just below */
  /* step 1 : cast to int */
  mm = _mm_cvttps_epi32(fx);
  /* step 2 : cast back to float */
  tmp = _mm_cvtepi32_ps(mm);
  /* if greater, substract 1 */
  v4sf mask = _mm_cmpgt_ps(tmp, fx);    
  mask = _mm_and_ps(mask, one);
  fx = _mm_sub_ps(tmp, mask);

  tmp = _mm_mul_ps(fx, *(v4sf*)_ps_cephes_exp_C1);
  v4sf z = _mm_mul_ps(fx, *(v4sf*)_ps_cephes_exp_C2);
  x = _mm_sub_ps(x, tmp);
  x = _mm_sub_ps(x, z);

  z = _mm_mul_ps(x,x);
  
  v4sf y = *(v4sf*)_ps_cephes_exp_p0;
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_exp_p1);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_exp_p2);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_exp_p3);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_exp_p4);
  y = _mm_mul_ps(y, x);
  y = _mm_add_ps(y, *(v4sf*)_ps_cephes_exp_p5);
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, x);
  y = _mm_add_ps(y, one);

  /* build 2^n */
  mm = _mm_cvttps_epi32(fx);
  // FIXME again, is it faster to load the value on the fly or from memory?
  mm = _mm_add_epi32(mm, _mm_set1_epi32(0x7f));
  mm = _mm_slli_epi32(mm, 23);

  const v4sf pow2n = *(v4sf*)&mm;
  
  y = _mm_mul_ps(y, pow2n);
  return y;
}
