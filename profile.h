#ifndef _PROFILE_H
#define _PROFILE_H

#define N_TIMING_SAMPLES 1000

typedef struct {
    int profiling_state;
    float dts[N_TIMING_SAMPLES];
} profile_t;

void init_profile(profile_t* profile);
void tick_profile(profile_t* profile, float delta, int frame_ctr);

#endif // _PROFILE_H
