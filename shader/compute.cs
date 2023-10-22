#version 430 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D stateOut;
layout(binding = 1) uniform sampler2D insol_LUT;

layout(location = 0) uniform float t;
layout(location = 1) uniform float dt;

// physical constants
const float pi            =     3.14159265;
const float days_per_year =   365.2422f;
const float secs_per_day  = 86400.0f;

// albedo parameters
const float a0 =   0.33f;
const float a2 =   0.25f;

// OLR parameters
const float olr_A = 210.0f;
const float olr_B =   2.0f;

float deg2rad(float x) {
    return pi / 180.0f * x;
}

float calcP2(float x) {
    return 0.5 * (3 * x * x - 1.0);
}

float calc_Cval(float depth) {
    return 4181.3 * 1.0e3 * depth;
}

float calc_Q(float lat, float lon, float day) {
    vec2 coord  = vec2(day / days_per_year, (lat + 90.0f) / 180.0f);
    vec4 soldat = texture(insol_LUT, coord); // 0.0f, S0*b2/a2, 0.0f, delta

    float phi    = deg2rad(lat);
    float h      = (mod((mod(day, 1.0) + (lon / 360)), 1.0) - 0.5) * 2 * pi;
    float coszen = sin(phi)*sin(soldat.a) + cos(phi)*cos(soldat.a)*cos(h);
    float Fsw    = soldat.g * coszen;
    Fsw          = Fsw * float(Fsw > 0);

    return Fsw;
}

float calc_albedo(float Ts, float lat) {
    float phi = deg2rad(lat);
    return a0 + a2 * calcP2(sin(phi));
}

float calc_Ts(float albedo, float Q, float T_old, float C_val) {
    float ASR = (1 - albedo) * Q;
    float OLR = olr_A + (olr_B * (T_old - 273.15));
    return (1 / C_val) * (ASR - OLR);
}

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    // Ts, q, alpha, Q
    //  r, g,     b, a
    vec4 value = imageLoad(stateOut, texelCoord);

    // coordinates
    float day = t; //mod(t, days_per_year);
    float lat = (float(gl_GlobalInvocationID.y + 0.5) / float(gl_NumWorkGroups.y * gl_WorkGroupSize.y) * 180.0f) - 90.0f;
    float lon = (float(gl_GlobalInvocationID.x + 0.5) / float(gl_NumWorkGroups.x * gl_WorkGroupSize.x) * 360.0f);

    // compute instant insolation
    float Q = calc_Q(lat, lon, day);

    // compute albedo
    float alpha = calc_albedo(value.r, lat);
    //clamp(alpha, 0, 1);

    // emit albedo as a and insol as b
    value.a = alpha;
    value.b = Q;

    // calculate water depth
    float C_val = calc_Cval(30.0);

    // compute temperature
    float dTdt = calc_Ts(alpha, Q, value.r, C_val);
    value.g = dTdt;
    value.r = value.r + (dTdt * dt * secs_per_day);

    imageStore(stateOut, texelCoord, value);
}
