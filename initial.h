#ifndef _INITIAL_H
#define _INITIAL_H

#include <stddef.h>
#include "nctools.h"

float* make_2d_initial(int nx, int ny);
unsigned int make_solar_table();
void make_LUTs(size_t model_width, size_t model_height,
    model_initial_t* model, unsigned int* LUT1, unsigned int* LUT2);

#endif
