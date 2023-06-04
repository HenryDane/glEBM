#include "fetch.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>

void mmm(float x, float* min, float* max, float* mean) {
    if (x < *min) *min = x;
    if (x > *max) *max = x;
    *mean += x;
}

void fetch_2d_state(unsigned int texture, int nx, int ny, float* Tmax,
    float* Tmin) {
    // create a buffer
    float* data = malloc(nx * ny * 4 * sizeof(float));

    // read texture into buffer
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data);

    // prepare temp search
    float Tmean = 0.0f;
    (*Tmax)     = 0.0f;
    (*Tmin)     = 1.0e9f;

    // prepare albedo search
    float amean = 0.0f;
    float amax  = 0.0f;
    float amin  = 1.0e9f;

    // prepare insolation search
    float Qmean = 0.0f;
    float Qmax  = 0.0f;
    float Qmin  = 1.0e9f;

    // prepare to process 4th channel
//    float dtmean = 0.0f;
//    float dtmax  = 0.0f;
//    float dtmin  = 1.0e9f;

    // explore texture
    for (size_t i = 0; i < nx * ny; i++) {
        mmm(data[(i*4) + 0], &Qmin, &Qmax, &Qmean);
        mmm(data[(i*4) + 1], &amin, &amax, &amean);
        mmm(data[(i*4) + 2], Tmin, Tmax, &Tmean);
//        mmm(data[(i*4) + 3], &dtmin, &dtmax, &dtmean);
    }

    Tmean = Tmean / ((float) nx * ny);
    Qmean = Qmean / ((float) nx * ny);
    amean = amean / ((float) nx * ny);
//    dtmean = dtmean / ((float) nx * ny);

#ifndef REDUCED_OUTPUT
    printf("  Temperature min=%.4f max=%.4f mean=%.4f\n", *Tmin, *Tmax, Tmean);
    printf("  Albedo      min=%.4f max=%.4f mean=%.4f\n", amin, amax, amean);
    printf("  Insolation  min=%.4f max=%.4f mean=%.4f\n", Qmin, Qmax, Qmean);
//    printf("  dt          min=%.4f max=%.4f mean=%.4f\n", dtmin, dtmax, dtmean);
#else
    printf("%.4f %.4f %.4f\n", *Tmin, *Tmax, Tmean);
#endif // REDUCED_OUTPUT
}
