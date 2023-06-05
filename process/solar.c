#include "solar.h"
#include "../common.h"
#include <math.h>
#include <errno.h>

float solar_lon(float ecc, float long_peri_rad, float day) {
    float delta_lambda = (day - 80.) * 2*pi/days_per_year;
    float ecc2 = ecc * ecc;
    float ecc3 = ecc2 * ecc;
    float beta = sqrt(1 - ecc2);
    float lambda_long_m = -2*((ecc/2 + (ecc3)/8 ) * (1+beta) * sin(-long_peri_rad) -
        (ecc2)/4 * (1/2 + beta) * sin(-2*long_peri_rad) + (ecc3)/8 *
        (1/3 + beta) * sin(-3*long_peri_rad)) + delta_lambda;
    float lambda_long = ( lambda_long_m + (2*ecc - (ecc3)/4)*sin(lambda_long_m - long_peri_rad) +
        (5/4)*(ecc2) * sin(2*(lambda_long_m - long_peri_rad)) + (13/12)*(ecc3)
        * sin(3*(lambda_long_m - long_peri_rad)) );
    return lambda_long;
}

float instant_insol(float lat, float lon, float day, float ecc,
                    float obliquity, float lambda_long, float long_peri) {
    float phi = deg2rad(lat);
    float delta = asin(sin(deg2rad(obliquity)) * sin(lambda_long));
    float h = (fmod((fmod(day, 1.0) + (lon / 360)), 1.0) - 0.5) * 2 * pi;
    float Ho = fabs(delta)-pi/2+fabs(phi) < 0 ? acos(-tan(phi)*tan(delta)) : (phi*delta>0. ? pi : 0.0f);
    float coszen = sin(phi)*sin(delta) + cos(phi)*cos(delta)*cos(h);
    float a1 = (1-ecc*ecc);
    float a2 = a1 * a1;
    float b1 = (1+ecc*cos(lambda_long - deg2rad(long_peri)));
    float b2 = b1 * b1;
    float Fsw = (fabs(h) < Ho) ? S0*( b2 / a2 * coszen) : 0.0f;
    Fsw = (Fsw > 0.0f) ? Fsw : 0.0f;

    return Fsw;
}

float calc_H0(float lat, float obliquity, float lambda_long) {
    float phi = deg2rad(lat);
    float delta = asin(sin(deg2rad(obliquity)) * sin(lambda_long));
    float H0;
    if (fabs(delta)-(pi/2)+fabs(phi) < 0) {
        H0 = acos(-tan(phi)*tan(delta));
    } else {
        if (phi*delta>0.0) {
            H0 = pi;
        } else {
            H0 = 0.0f;
        }
    }

    return H0;
}

float a2_b2_ratio(float ecc, float lambda_long, float long_peri_rad) {
    float a1 = (1-ecc*ecc);
    float a2 = a1 * a1;
    float b1 = (1+ecc*cos(lambda_long - long_peri_rad));
    float b2 = b1 * b1;

    return S0 * b2 / a2;
}
