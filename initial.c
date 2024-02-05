#include "initial.h"
#include <stdlib.h>
#include "process/solar.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include "nctools.h"

float* make_2d_initial(int nx, int ny) {
    float* data = (float*) malloc(nx * ny * 4 * sizeof(float));

    for (size_t y = 0; y < ny; y++) {
        for (size_t x = 0; x < nx; x++) {
            size_t i = (y * nx) + x;

            // temperature
            data[(i * 4) + 0] = 273.15;
            data[(i * 4) + 1] =   0.0f;  // vapor mmr
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

            data[((y * nx) + x) * 4 + 0] = abra;
            data[((y * nx) + x) * 4 + 1] = delta;
            data[((y * nx) + x) * 4 + 2] = 0.0f;
            data[((y * nx) + x) * 4 + 3] = 0.0f;
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

void make_LUTs(size_t model_width, size_t model_height,
    model_initial_t* model, unsigned int* LUT1, unsigned int* LUT2) {
    // allocate memory for LUT1
    float* data1 = (float*) malloc(
        model_width * model_height * 4 * sizeof(float));
        // allocate memory for LUT2
    float* data2 = (float*) malloc(
        model_width * model_height * 4 * sizeof(float));

    // copy stuff over
    for (size_t i = 0; i < model_width * model_height; i++) {
        data1[(i * 4) + 0] = model->lats[i / model_width];
        data1[(i * 4) + 1] = model->lons[i % model_width];
        data1[(i * 4) + 2] = model->Bs[i];
        data1[(i * 4) + 3] = model->As[i];
        data2[(i * 4) + 0] = model->a0s[i];
        data2[(i * 4) + 1] = model->a2s[i];
        data2[(i * 4) + 2] = model->ais[i];
        data2[(i * 4) + 3] = model->depths[i];
    }

    // gemerate textures
    glGenTextures(1, LUT1);
    glBindTexture(GL_TEXTURE_2D, *LUT1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, model_width, model_height, 0,
                 GL_RGBA, GL_FLOAT, data1);
    glGenTextures(1, LUT2);
    glBindTexture(GL_TEXTURE_2D, *LUT2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, model_width, model_height, 0,
                 GL_RGBA, GL_FLOAT, data2);
}
