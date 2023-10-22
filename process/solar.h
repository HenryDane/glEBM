#ifndef _SOLAR_H
#define _SOLAR_H

float solar_lon(float ecc, float long_peri_rad, float day);
float instant_insol(float lat, float lon, float day, float ecc,
                    float obliquity, float lambda_long, float long_peri);
float a2_b2_ratio(float ecc, float lambda_long, float long_peri_rad);

#endif // _SOLAR_H
