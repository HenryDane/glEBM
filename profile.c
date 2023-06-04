#include "profile.h"
#include <stdio.h>
#include <stdlib.h>

void init_profile(profile_t* profile) {
    profile->profiling_state = 0;
    for (size_t i = 0; i < N_TIMING_SAMPLES; profile->dts[i++] = 0.0f);
}

void tick_profile(profile_t* profile, float delta, int frame_ctr) {
    if (profile->profiling_state >= 0 && profile->profiling_state < N_TIMING_SAMPLES && frame_ctr > 120) {
        profile->dts[profile->profiling_state] = delta;
        profile->profiling_state++;

        if (profile->profiling_state >= N_TIMING_SAMPLES) {
            profile->profiling_state = -1;
        }
    }

    if (profile->profiling_state == -1) {
        profile->profiling_state = -2;

        float max = 0.0f;
        float min = 10000.0f;
        float mean = 0.0f;
        for (size_t i = 0; i < N_TIMING_SAMPLES; i++) {
            if (profile->dts[i] < min) {
                min = profile->dts[i];
            }
            if (profile->dts[i] > max) {
                max = profile->dts[i];
            }
            mean += profile->dts[i];
        }
        mean = mean / ((float) N_TIMING_SAMPLES) * 1000.0f;
        min *= 1000.0f;
        max *= 1000.0f;

        printf("mean=%.3f min=%.3f max=%.3f\n", mean, min, max);
    }
}
