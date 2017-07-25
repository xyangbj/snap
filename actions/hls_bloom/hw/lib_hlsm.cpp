/*****************************************************************************
 *
 *     Author: Xilinx, Inc.
 *
 *     This text contains proprietary, confidential information of
 *     Xilinx, Inc. , is distributed by under license from Xilinx,
 *     Inc., and may be used, copied and/or disclosed only pursuant to
 *     the terms of a valid license agreement with Xilinx, Inc.
 *
 *     XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS"
 *     AS A COURTESY TO YOU, SOLELY FOR USE IN DEVELOPING PROGRAMS AND
 *     SOLUTIONS FOR XILINX DEVICES.  BY PROVIDING THIS DESIGN, CODE,
 *     OR INFORMATION AS ONE POSSIBLE IMPLEMENTATION OF THIS FEATURE,
 *     APPLICATION OR STANDARD, XILINX IS MAKING NO REPRESENTATION
 *     THAT THIS IMPLEMENTATION IS FREE FROM ANY CLAIMS OF INFRINGEMENT,
 *     AND YOU ARE RESPONSIBLE FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE
 *     FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY DISCLAIMS ANY
 *     WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE
 *     IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR
 *     REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF
 *     INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE.
 *
 *     Xilinx products are not intended for use in life support appliances,
 *     devices, or systems. Use in such applications is expressly prohibited.
 *
 *     (c) Copyright 2012-2013 Xilinx Inc.
 *     All rights reserved.
 *
 *****************************************************************************/

#include <cmath>
#include "hls_math.h"

extern "C" {

#ifdef _WIN32
#define NOTHROW
#else
#define NOTHROW throw()
#endif

double sin(double x) {
    return hls::sin(x);
}

float sinf(float x) {
    return hls::sinf(x);
}

float sinpif(float x) {
    return hls::sinpif(x);
}

double cos(double x) {
    return hls::cos(x);
}

float cosf(float x) {
    return hls::cosf(x);
}

float cospif(float x) {
    return hls::cospif(x);
}

/*
double atan(double x) {
    return hls::atan(x);
}
*/

float atanf(float x) {
    return hls::atanf(x);
}

double atan2(double y, double x) {
    return hls::atan2(y, x);
}

float atan2f(float y, float x) {
    return hls::atan2f(y, x);
}

float sinhf(float x) {
	return hls::sinhf(x);
}

float coshf(float x) {
	return hls::coshf(x);
}

void sincos(double x, double *sin, double *cos) {
    hls::sincos(x, sin, cos);
}

void sincosf(float x, float *sin, float *cos) {
    hls::sincosf(x, sin, cos);
}

double tan(double x) {
    return hls::tan(x);
}

float tanf(float x) {
    return hls::tanf(x);
}

double copysign(double x, double y) {
    return hls::copysign(x, y);
}

float copysignf(float x, float y) {
    return hls::copysignf(x, y);
}

double fmax(double x, double y) {
    return hls::fmax(x, y);
}

float fmaxf(float x, float y) {
    return hls::fmaxf(x, y);
}

double fmin(double x, double y) {
    return hls::fmin(x, y);
}

float fminf(float x, float y) {
    return hls::fminf(x, y);
}

double fabs(double x) {
    return hls::fabs(x);
}

float fabsf(float x) {
    return hls::fabsf(x);
}

double floor(double x) {
    return hls::floor(x);
}

float floorf(float x) {
    return hls::floorf(x);
}

double ceil(double x) {
    return hls::ceil(x);
}

float ceilf(float x) {
    return hls::ceilf(x);
}

double trunc(double x) {
    return hls::trunc(x);
}

float truncf(float x) {
    return hls::truncf(x);
}

double round(double x) {
    return hls::round(x);
}

float roundf(float x) {
    return hls::roundf(x);
}

long int lrint(double x) {
    return hls::lrint(x);
}

long long int llrint(double x) {
    return hls::llrint(x);
}

long int lrintf(float x) {
    return hls::lrintf(x);
}

long long int llrintf(float x) {
    return hls::llrintf(x);
}

double modf(double x, double *intpart) NOTHROW {
    return hls::modf(x, intpart);
}

float modff(float x, float *intpart) NOTHROW {
    return hls::modff(x, intpart);
}

double fract(double x, double *intpart) NOTHROW {
    return hls::fract(x, intpart);
}

float fractf(float x, float *intpart) NOTHROW {
    return hls::fractf(x, intpart);
}

double frexp (double x, int* exp) {
    return hls::frexp(x, exp);
}

float frexpf (float x, int* exp) {
    return hls::frexpf(x, exp);
}

double ldexp (double x, int exp) {
    return hls::ldexp(x, exp);
}

float ldexpf (float x, int exp) {
    return hls::ldexpf(x, exp);
}

#if (defined AESL_SYN || defined HLS_NO_XIL_FPO_LIB)
// These functions are now implemented by libm in simulation.
// These functions are now implemented by intrinsic during synthesis.
#else
double log(double x) {
    return hls::log(x);
}

float logf(float x) {
    return hls::logf(x);
}

double sqrt(double x) {
    return hls::sqrt(x);
}

float sqrtf(float x) {
    return hls::sqrtf(x);
}

double exp(double x) {
    return hls::exp(x);
}

float expf(float x) {
    return hls::expf(x);
}
#endif

double log10(double x) {
    return hls::log10(x);
}

float log10f(float x) {
    return hls::log10f(x);
}

double recip(double x) {
    return hls::recip(x);
}

float recipf(float x) {
    return hls::recipf(x);
}

double rsqrt(double x) {
    return hls::rsqrt(x);
}

float rsqrtf(float x) {
    return hls::rsqrtf(x);
}

int __signbitf(float t_in) {
    return hls::__signbit(t_in);
}

int __signbit(double t_in) {
    return hls::__signbit(t_in);
}

int __finitef(float t_in) {
	return hls::__isfinite(t_in);
}

int __finite(double t_in) {
	return hls::__isfinite(t_in);
}

int __isinff(float t_in) {
	return hls::__isinf(t_in);
}

int __isinf(double t_in) {
	return hls::__isinf(t_in);
}

int isinf(double t_in) {
	return hls::__isinf(t_in);
}

int __isnanf(float t_in) {
	return hls::__isnan(t_in);
}

int __isnan(double t_in) {
	return hls::__isnan(t_in);
}

int isnan(double t_in) {
	return hls::__isnan(t_in);
}

int __isnormalf(float t_in) {
	return hls::__isnormal(t_in);
}

int __isnormal(double t_in) {
	return hls::__isnormal(t_in);
}

int __fpclassifyf(float t_in) NOTHROW {
	return hls::__fpclassifyf(t_in);
}

int __fpclassify(double t_in) NOTHROW {
	return hls::__fpclassify(t_in);
}

double nan(const char *p) {
    return hls::nan(p);
}

float nanf(const char *p) {
    return hls::nanf(p);
}

} // extern "C"

// XSIP watermark, do not delete 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
