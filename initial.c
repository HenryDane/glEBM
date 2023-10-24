#include "initial.h"
#include <stdlib.h>
#include "process/solar.h"
#include "common.h"
#include <math.h>
#include <stdio.h>

float calcP2(float x) {
    return 0.5 * (3 * x * x - 1.0);
}

float* make_2d_initial(int nx, int ny) {
    float* data = (float*) malloc(nx * ny * 4 * sizeof(float));

    // must be > 0C;
    const float Ti   = 293.15;
    const float c0H0 =   2.0e8;
    const float T0   =  11.0;
    const float T2   = -10.0;

    for (size_t y = 0; y < ny; y++) {
        for (size_t x = 0; x < nx; x++) {
            size_t i = (y * nx) + x;
            float lat = (((float) y + 0.5) / ((float) ny) * 180.0f) - 90.0f;
            float sinphi = sin(deg2rad(lat));
            float T = T0 + T2 * calcP2(sinphi);

            // convert units
            T += 273.15;

            // temperature
            data[(i * 4) + 0] = T;
            data[(i * 4) + 1] = c0H0 * (T - 273.15);  // vapor mmr
            data[(i * 4) + 2] =   0.0f;  // meridional wind
            data[(i * 4) + 3] =   0.0f;  // zonal wind
        }
    }

    return data;
}

unsigned int make_solar_table() {
    const int nx = 512;
    const int ny = 512;
    unsigned int solar_LUT;
    float* data = (float*) malloc(nx * ny * 4 * sizeof(float));
    for (size_t i = 0; i < nx * ny * 4; data[i++] = 0.0f);

    float long_peri_rad = deg2rad(long_peri);

    for (size_t x = 0; x < nx; x++) {
        for (size_t y = 0; y < ny; y++) {
            float lat = (((float) y + 0.5) / ((float) ny) * 180.0f) - 90.0f;
            float day = ((float) x) / ((float) nx) * days_per_year;

            float slon = solar_lon(ecc, long_peri_rad, day);
            float abra = a2_b2_ratio(ecc, slon, long_peri_rad);
            float delta = asin(sin(deg2rad(obliquity)) * sin(slon));

            data[((y * nx) + x) * 4 + 0] = 0.0f;
            data[((y * nx) + x) * 4 + 1] = abra;
            data[((y * nx) + x) * 4 + 2] = 0.0f;
            data[((y * nx) + x) * 4 + 3] = delta;
        }
    }

    glGenTextures(1, &solar_LUT);
    glBindTexture(GL_TEXTURE_2D, solar_LUT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nx, ny, 0,
                 GL_RGBA, GL_FLOAT, data);

    return solar_LUT;
}
