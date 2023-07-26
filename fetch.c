#include "fetch.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

void fetch_2d_state(unsigned int texture, int nx, int ny, float* Tmax,
    float* Tmin, float* qmax, float* qmin, float* umax, float* umin,
    float* vmax, float* vmin) {
    // create a buffer
    float* data = malloc(nx * ny * 4 * sizeof(float));

    // read texture into buffer
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data);

    // prepare search
    float Tmean = 0.0f;
    float qmean = 0.0f;
    float umean = 0.0f;
    float vmean = 0.0f;

    // explore texture
    for (size_t i = 0; i < nx * ny; i++) {
        mmm(data[(i * 4) + 0], Tmin, Tmax, &Tmean);
        mmm(data[(i * 4) + 1], qmin, qmax, &qmean);
        mmm(data[(i * 4) + 2], umin, umax, &umean);
        mmm(data[(i * 4) + 3], vmin, vmax, &vmean);
    }

    // convert sums to means
    Tmean = Tmean / ((float) nx * ny);
    qmean = qmean / ((float) nx * ny);
    umean = umean / ((float) nx * ny);
    vmean = vmean / ((float) nx * ny);

#ifndef REDUCED_OUTPUT
    printf("  Temperature  min=%.4f max=%.4f mean=%.4f\n", *Tmin, *Tmax, Tmean);
    printf("  Vapor MMR    min=%.4f max=%.4f mean=%.4f\n", *qmin, *qmax, qmean);
    printf("  Merid. Wind  min=%.4e max=%.4e mean=%.4e\n", *umin, *umax, umean);
    printf("  Zonal Wind   min=%.4e max=%.4e mean=%.4e\n", *vmin, *vmax, vmean);
#else
    printf("%.4f %.4f %.4f\n", *Tmin, *Tmax, Tmean);
#endif // REDUCED_OUTPUT
}

void fetch_and_dump_state(unsigned int surf_texture, int nx, int ny,
    const char* path) {
    // create buffers
    char fname[256];
    float* data = malloc(nx * ny * 4 * sizeof(float));

    // read texture into buffer
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data);

    // try to make folder
    struct stat st = {0};

    if (stat("results/", &st) == -1) {
        mkdir("results/", 0777);
    }

    // print csv values for latitude
    sprintf(fname, "results/latitudes_%dx%d.csv", nx, ny);
    FILE* file = fopen(fname, "w");
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            float lat = (((float)(y + 0.5)) / ((float)ny) * 180.0f) - 90.0f;
            fprintf(file, "%.4e, ", lat);
        }
        fprintf(file, "\n");
    }
    fclose(file);

    // print csv values for lon
    sprintf(fname, "results/longitudes_%dx%d.csv", nx, ny);
    file = fopen(fname, "w");
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            float lon = (((float)(x + 0.5)) / ((float)nx) * 360.0f);
            fprintf(file, "%.4e, ", lon);
        }
        fprintf(file, "\n");
    }
    fclose(file);

    // print temperature values
    sprintf(fname, "results/temperatures_%dx%d.csv", nx, ny);
    file = fopen(fname, "w");
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            int i = (y * nx) + x;
            float temp = data[(i * 4) + 0];
            fprintf(file, "%.4e, ", temp);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}
