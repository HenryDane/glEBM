#ifndef _NCTOOLS_H
#define _NCTOOLS_H

#include <stddef.h>

typedef struct {
    float *lats, *lons;
    float *Ts, *Bs, *depths, *a0s, *a2s, *ais;
} model_initial_t;

// linked list time oh yeah
typedef struct {
    struct storage_frame_t* next;
    struct storage_frame_t* prev;
    float time;
    float* data;
} storage_frame_t;

typedef struct {
    int n_timesteps, n_slots;
    float final_time, timestep;
    storage_frame_t* head;
} model_storage_t;

void init_model_storage(model_storage_t* model, float final_time,
    int model_width, int model_height);
void model_storage_add_frame(model_storage_t* model, float time, float* data);
void model_storage_write(int size_x, int size_y, model_storage_t* model,
    model_initial_t* initial, const char* path);
void model_storage_free(model_storage_t* model);

void read_input(size_t* model_width, size_t* model_height, model_initial_t* m);

#endif
