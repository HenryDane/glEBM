#ifndef _COMMON_H
#define _COMMON_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define DEFAULT_MODEL_WIDTH  192
#define DEFAULT_MODEL_HEIGHT 288

#define SCR_WIDTH  DEFAULT_MODEL_WIDTH
#define SCR_HEIGHT DEFAULT_MODEL_HEIGHT

#define DEFAULT_RESULT_PATH "results/"

//#define REDUCED_OUTPUT

float deg2rad(float x);
void mmm(float x, float* min, float* max, float* mean);

// physical constants
extern const float pi, days_per_year, S0, ecc, obliquity, long_peri;

#endif // _COMMON_H
