#ifndef _FETCH_H
#define _FETCH_H

float* fetch_2d_state(unsigned int texture, int nx, int ny, float* Tmax,
    float* Tmin, float* qmax, float* qmin, float* umax, float* umin,
    float* vmax, float* vmin);

void fetch_and_dump_state(unsigned int surf_texture, int nx, int ny,
    const char* path);

#endif
