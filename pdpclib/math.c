/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards, 3:711/934@fidonet.         */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  math.c - implementation of stuff in math.h                       */
/*                                                                   */
/*********************************************************************/

#include "math.h"

double ceil(double x)
{
    int y;
    
    y = (int)x;
    if ((double)y < x)
    {
        y++;
    }
    return ((double)y);
}

#ifdef fabs
#undef fabs
#endif
double fabs(double x)
{
    if (x < 0.0)
    {
        x = -x;
    }
    return (x);
}

double floor(double x)
{
    int y;
    
    if (x < 0.0)
    {
        y = (int)fabs(x);
        if ((double)y != x)
        {
            y--;
        }
    }
    else
    {
        y = (int)x;
    }
    return ((double)x);
}

double fmod(double x, double y)
{
    return (x / y);
}

static double i_acos(double x)
{
    return (acos(x));
}

static double i_asin(double x)
{
    return (asin(x));
}

static double i_atan(double x)
{
    return (atan(x));
}

static double i_atan2(double y, double x)
{
    return (atan2(y,x));
}

static double i_cos(double x)
{
    return (cos(x));
}

static double i_sin(double x)
{
    return (sin(x));
}

static double i_tan(double x)
{
    return (tan(x));
}

static double i_cosh(double x)
{
    return (cosh(x));
}

static double i_sinh(double x)
{
    return (sinh(x));
}

static double i_tanh(double x)
{
    return (tanh(x));
}

static double i_exp(double x)
{
    return (exp(x));
}

static double i_log(double x)
{
    return (log(x));
}

static double i_log10(double x)
{
    return (log10(x));
}

static double i_pow(double x, double y)
{
    return (pow(x, y));
}

static double i_sqrt(double x)
{
    return (sqrt(x));
}

#ifdef acos
#undef acos
#endif
double acos(double x)
{
    return (i_acos(x));
}

#ifdef asin
#undef asin
#endif
double asin(double x)
{
    return (i_asin(x));
}

#ifdef atan
#undef atan
#endif
double atan(double x)
{
    return (i_atan(x));
}

double atan2(double y, double x)
{
    return (i_atan2(y, x));
}

#ifdef cos
#undef cos
#endif
double cos(double x)
{
    return (i_cos(x));
}

#ifdef sin
#undef sin
#endif
double sin(double x)
{
    return (i_sin(x));
}

#ifdef tan
#undef tan
#endif
double tan(double x)
{
    return (i_tan(x));
}

double cosh(double x)
{
    return (i_cosh(x));
}

double sinh(double x)
{
    return (i_sinh(x));
}

double tanh(double x)
{
    return (i_tanh(x));
}

double exp(double x)
{
    return (i_exp(x));
}

double log(double x)
{
    return (i_log(x));
}

double log10(double x)
{
    return (i_log10(x));
}

double pow(double x, double y)
{
    return (i_pow(x, y));
}

#ifdef sqrt
#undef sqrt
#endif
double sqrt(double x)
{
    return (i_sqrt(x));
}
