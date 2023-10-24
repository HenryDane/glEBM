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
const float Re            =     6.373e6f;

// albedo parameters
//const float a0 =   0.3f;
//const float a2 =   0.078f;
//const float ai =   0.62f;
//const float Tf = 263.15f;

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
//    float is_freezing = float(Tf > Ts);
//    float albedo = 0;
//    albedo += is_freezing * ai;
//    albedo += (1 - is_freezing) * (a0 + a2 * calcP2(phi));
//    return albedo;

    const float a0 =   0.33f;
const float a2 =   0.25f;
    return a0 + a2 * calcP2(sin(phi));
}

float calc_Ts(float albedo, float Q, float T_old, float C_val) {
    float ASR = (1 - albedo) * Q;
    float OLR = olr_A + (olr_B * (T_old - 273.15));
    return (1 / C_val) * (ASR - OLR);
}

float calc_clamp_fact(ivec2 coord) {
    ivec2 imgsize = imageSize(stateOut);
    float clamp_fact = //float(coord.x >= 0) + float(coord.x < imgsize.x) +
        float(coord.y >= 0) + float(coord.y < imgsize.y);
    return clamp(1 - clamp_fact, 0, 1);
}

vec4 safeRead(ivec2 coord) {
    ivec2 imgsize = imageSize(stateOut);
//    coord.y = clamp(coord.y, 0, imgsize.y - 1);
//    ivec2 tc = ivec2(mod(coord + imgsize, imageSize(stateOut)));
//
//    return imageLoad(stateOut, tc);
    float clamp_fact = calc_clamp_fact(coord);
    return mix(vec4(285.3, 0, 0, 0), imageLoad(stateOut, coord), clamp_fact);
//    ivec2 clamped_coord = ivec2(coord.x, clamp(coord.y, 0, imgsize.y-1));
//    vec4 clamped = imageLoad(stateOut, clamped_coord);
//    return mix(vec4(clamped.r, 0, 0, 0), imageLoad(stateOut, coord), clamp_fact);
}

vec4 calc_adv_diff(vec4 S, ivec2 tcoord, float C_val, float lat) {
    ivec2 imgsize = imageSize(stateOut);
    // define diffusivity
    const float D = 0.555;
    float K = D / C_val * Re * Re;

    // calculate dlambda, dphi
    float dlambda = 2.0 * pi / float(gl_NumWorkGroups.x * gl_WorkGroupSize.x);
    float dphi    = pi / float(gl_NumWorkGroups.y * gl_WorkGroupSize.y);

    // calculate dx, dy from (4.1)
    float dx = Re * cos(deg2rad(lat)) * dlambda;
    float dy = Re * dphi;

    // get nearby elements
    vec4 Sipj  = safeRead(tcoord + ivec2( 1,  0));
    vec4 Simj  = safeRead(tcoord + ivec2(-1,  0));
    vec4 Sijp  = safeRead(tcoord + ivec2( 0,  1));
    vec4 Sijpp = safeRead(tcoord + ivec2( 0,  2));
    vec4 Sijm  = safeRead(tcoord + ivec2( 0, -1));
    vec4 Sijmm = safeRead(tcoord + ivec2( 0, -2));

    // 1 if N/S edge
    float calc_1x = float(tcoord.y > 0) * float(tcoord.y < imgsize.y - 1);
//    calc_1x = 1 - calc_1x;

    vec4 dNdtKX = Sipj + Simj - S - S;
    dNdtKX *= K;
    dNdtKX /= dx;
    dNdtKX /= dx;
    dNdtKX /= (dt * secs_per_day);

//    vec4 dNdtKY = Sijp + Sijm - S - S;
    vec4 dNdtKY  = Sijp + Sijm - S - S;
    dNdtKY *= K;
    dNdtKY /= dy;
    dNdtKY /= dy;
    dNdtKY /= (dt * secs_per_day);

//    vec4 dNdt = dNdtKX + dNdtKY;
    vec4 dNdt = dNdtKY;
//    vec4 dNdt = K * dNdtKY;
//    dNdt = dNdt - S;
//    dNdt = dNdt / (dt * secs_per_day);

    return dNdt;
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

    // emit albedo as a and insol as b
    value.a = alpha;
    value.b = Q;

    // calculate water depth
    float C_val = calc_Cval(30.0);

    // compute temperature
    value.r += calc_Ts(alpha, Q, value.r, C_val) * dt * secs_per_day;

    // adv diff
    float dTdt = calc_adv_diff(value, texelCoord, C_val, lat).r;
    value.g = dTdt;
    value.r += dTdt * dt * secs_per_day;

    imageStore(stateOut, texelCoord, value);
}
