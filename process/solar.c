#include "solar.h"
#include "../common.h"
#include <math.h>
#include <errno.h>

float solar_lon(float ecc, float long_peri_rad, float day) {
    float delta_lambda = (day - 80.0f) * 2 * pi / days_per_year;
    float ecc2 = ecc * ecc;
    float ecc3 = ecc2 * ecc;
    float beta = sqrt(1 - ecc2);
    float lambda_long_m = -2.0f * ((ecc/2.0f + ecc3/8.0f ) * (1.0f+beta) * sin(-long_peri_rad) -
        (ecc2/4.0f) * (0.5f + beta) * sin(-2.0f*long_peri_rad) + (ecc3/8.0f) *
        (0.333333333f + beta) * sin(-3*long_peri_rad)) + delta_lambda;
    float lambda_long = ( lambda_long_m + (2*ecc - (ecc3/4.0f))*sin(lambda_long_m - long_peri_rad) +
        (1.25f*ecc2) * sin(2*(lambda_long_m - long_peri_rad)) + (1.08333333333f*ecc3)
        * sin(3.0f*(lambda_long_m - long_peri_rad)) );
    return lambda_long;
}

float a2_b2_ratio(float ecc, float lambda_long, float long_peri_rad) {
    float a1 = (1-ecc*ecc);
    float a2 = a1 * a1;
    float b1 = (1+ecc*cos(lambda_long - long_peri_rad));
    float b2 = b1 * b1;

    return S0 * b2 / a2;
}
