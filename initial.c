#include "initial.h"
#include <stdlib.h>

float* make_2d_initial(int nx, int ny) {
    float* data = (float*) malloc(nx * ny * 4 * sizeof(float));

    for (size_t i = 0; i < nx * ny; i++) {
        data[i * 4 + 0] =   0.0f; // insolation
        data[i * 4 + 1] =   0.1f; // albedo
        data[i * 4 + 2] = 270.0f; // temperature
        data[i * 4 + 3] =   0.0f; // unused
    }

    return data;
}
