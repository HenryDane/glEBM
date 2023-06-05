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
const float pi            =    3.14159265;
const float days_per_year =  365.0f;
const float S0            = 1367.0f;

// orbital parameters
const float ecc       =   0.01724f;
const float obliquity =  23.45f;
const float long_peri = 281.4f;
