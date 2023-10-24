#include "common.h"

float deg2rad(float x) {
    return pi / 180.0f * x;
}

void mmm(float x, float* min, float* max, float* mean) {
    if (x < *min) *min = x;
    if (x > *max) *max = x;
    *mean += x;
}

// physical constants
const float pi            =    3.14159265f;
const float days_per_year =  365.2422f;
const float S0            = 1365.2f;

// orbital parameters
const float ecc       =   0.017236f;
const float obliquity =  23.446f;
const float long_peri = 281.37f;
