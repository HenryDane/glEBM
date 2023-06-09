#ifndef _COMMON_H
#define _COMMON_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define SCR_WIDTH  512
#define SCR_HEIGHT 512

#define MODEL_WIDTH  128
#define MODEL_HEIGHT 128

//#define REDUCED_OUTPUT

float deg2rad(float x);
void mmm(float x, float* min, float* max, float* mean);

// physical constants
extern const float pi, days_per_year, S0, ecc, obliquity, long_peri;

#endif // _COMMON_H
