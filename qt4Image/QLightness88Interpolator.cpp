/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#include "QLightness88Interpolator.h"

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
# include <cfloat>
#endif


namespace
{

// Workaround since Visual Studio doesn't have the C99 cbrt function
// Taken via Google code search from:
//  http://mingw-w64.svn.sourceforge.net/svnroot/mingw-w64›trunk›mingw-w64-crt›math›cbrt.c
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)

/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

static const double CBRT2  = 1.2599210498948731647672;
static const double CBRT4  = 1.5874010519681994747517;
static const double CBRT2I = 0.79370052598409973737585;
static const double CBRT4I = 0.62996052494743658238361;

double cbrt(double x)
{
        int e, rem, sign;
        double z;

        if (x == 0.0 || _finite(x) == 0)
                return (x);

        if (x > 0)
                sign = 1;
        else
        {
                sign = -1;
                x = -x;
        }

        z = x;
        /* extract power of 2, leaving
         * mantissa between 0.5 and 1
         */
        x = frexp(x, &e);

        /* Approximate cube root of number between .5 and 1,
         * peak relative error = 9.2e-6
         */
        x = (((-1.3466110473359520655053e-1  * x
              + 5.4664601366395524503440e-1) * x
              - 9.5438224771509446525043e-1) * x
              + 1.1399983354717293273738e0 ) * x
              + 4.0238979564544752126924e-1;

        /* exponent divided by 3 */
        if (e >= 0)
        {
                rem = e;
                e /= 3;
                rem -= 3*e;
                if (rem == 1)
                        x *= CBRT2;
                else if (rem == 2)
                        x *= CBRT4;
        }
        /* argument less than 1 */
        else
        {
                e = -e;
                rem = e;
                e /= 3;
                rem -= 3*e;
                if (rem == 1)
                        x *= CBRT2I;
                else if (rem == 2)
                        x *= CBRT4I;
                e = -e;
        }

        /* multiply by power of 2 */
        x = ldexp(x, e);

        /* Newton iteration */
        x -= ( x - (z/(x*x)) )*0.33333333333333333333;
        x -= ( x - (z/(x*x)) )*0.33333333333333333333;

        if (sign < 0)
                x = -x;
        return (x);
}

#endif // defined(_MSC_VER) && !defined(__INTEL_COMPILER)



// LUT for getting the key, given a slider position in the range [0,88]
// Since the function is smooth, this could be used as control points
// for interpolating intermediate positions.
const double LUT_L_inv[] = {0,
0.001258018704408470, 0.002516037408816941, 0.003774056113225411,
0.005032074817633881, 0.006290093522042351, 0.007548112226450822,
0.008806130930859292, 0.01011987674481842, 0.01155807289904176,
0.01312645528928362, 0.01483066456701792, 0.01667634138371861,
0.01866912639085963, 0.02081466023991491, 0.02311858358235840,
0.02558653706966403, 0.02822416135330575, 0.03103709708475748,
0.03403098491549318, 0.03721146549698678, 0.04058417948071221,
0.04415476751814343, 0.04792887026075435, 0.05191212836001894,
0.05611018246741112, 0.06052867323440483, 0.06517324131247401,
0.07004952735309260, 0.07516317200773455, 0.08051981592787378,
0.08612509976498424, 0.09198466417053987, 0.09810414979601460,
0.1044891972928824, 0.1111454473126171, 0.1180785405066928,
0.1252941175265834, 0.1327978190237627, 0.1405952856497048,
0.1486921580558835, 0.1570940768937729, 0.1658066828148468,
0.1748356164705793, 0.1841865185124441, 0.1938650295919154,
0.2038767903604669, 0.2142274414695727, 0.2249226235707067,
0.2359679773153428, 0.2473691433549550, 0.2591317623410172,
0.2712614749250033, 0.2837639217583873, 0.2966447434926431,
0.3099095807792447, 0.3235640742696660, 0.3376138646153810,
0.3520645924678635, 0.3669218984785875, 0.3821914232990270,
0.3978788075806559, 0.4139896919749481, 0.4305297171333776,
0.4475045237074183, 0.4649197523485442, 0.4827810437082291,
0.5010940384379471, 0.5198643771891720, 0.5390977006133779,
0.5587996493620386, 0.5789758640866281, 0.5996319854386203,
0.6207736540694891, 0.6424065106307086, 0.6645361957737526,
0.6871683501500951, 0.7103086144112100, 0.7339626292085712,
0.7581360351936528, 0.7828344730179286, 0.8080635833328725,
0.8338290067899586, 0.8601363840406607, 0.8869913557364529,
0.9143995625288089, 0.9423666450692028, 0.9708982440091085,
1.000000000000000};


// Forward function: given a key (in the range [0,1]) returns
// the corresponding slider position in the range [0,88]
inline int L(const double key) {
    Q_ASSERT(0.0 <= key && key <= 1.0);
    const double value = key > 0.0088575412121709177397 ?
        -14.08 + 102.08*cbrt(key) : 794.90074074074074074*key;
    const int pos = qRound(value);
    Q_ASSERT(0 <= pos && pos <= 88);
    return pos;
}


// Helper inverse function using the LUT: given the slider position
// returns the key
inline double L_inv(const int pos) {
    Q_ASSERT(0 <= pos && pos <= 88);
    const double key = LUT_L_inv[pos];
    Q_ASSERT(0.0 <= key && key <= 1.0);
    return key;
}

} // namespace



QLightness88Interpolator::QLightness88Interpolator(QAbstractSlider *slider,
                                                   QLineEdit *edit,
                                                   QObject *parent)
: QInterpolator(0.0, 0.0, 1.0, slider, edit, parent)
{
    updateState(0.0, 0.0, 1.0, slider->minimum(), slider->maximum());
}


void QLightness88Interpolator::updateState(double minimum, double, double maximum,
                                           int sliderMinimum,
                                           int sliderMaximum)
{
    Q_ASSERT(minimum == 0.0 && maximum == 1.0);
    Q_ASSERT(sliderMinimum == 0 && sliderMaximum == 88);
}


int QLightness88Interpolator::toSliderValue(double value)
{
    return ::L(value);
}


double QLightness88Interpolator::toValue(int sliderValue)
{
    return ::L_inv(sliderValue);
}
