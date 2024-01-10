#version 430 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D stateOut;
layout(binding = 1) uniform sampler2D insol_LUT;
layout(binding = 2) uniform sampler2D physp_LUT;

layout(location = 0) uniform float t;
layout(location = 1) uniform float dt;

// physical constants
const float pi            =     3.14159265;
const float days_per_year =   365.2422f;
const float secs_per_day  = 86400.0f;
const float Re            =     6.373e6f;
const float Rd            =   287.0;
const float Rv            =   461.5;
const float Lh_vap        =     2.5e6;
const float gas_cp        =  1004.0;
const float eps = Rd / Rv;

// albedo parameters
const float a0 =   0.3f;
const float a2 =   0.078f;
const float ai =   0.62f;
const float Tf = 263.15f;

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
    float coszen = sin(phi)*sin(soldat.g) + cos(phi)*cos(soldat.g)*cos(h);
    float Fsw    = soldat.r * coszen;
    Fsw          = Fsw * float(Fsw > 0);

    return Fsw;
}

float calc_albedo(float Ts, float lat) {
    float phi = deg2rad(lat);
    float is_freezing = float(Tf > Ts);
    float albedo = 0;
    albedo += is_freezing * ai;
    albedo += (1 - is_freezing) * (a0 + a2 * calcP2(phi));
    return albedo;
}

float calc_Ts(float albedo, float Q, float T_old, float C_val) {
    float ASR = (1 - albedo) * Q;
    float OLR = olr_A + (olr_B * (T_old - 273.15));
    return (1 / C_val) * (ASR - OLR);
}

vec4 calc_merid_advdiff(vec4 N, ivec2 coord, float lat, float C, float f) {
    const float D = 0.555;
    ivec2 imgsize = imageSize(stateOut);

    float phi = deg2rad(lat);
    float dphi = pi / float(gl_NumWorkGroups.y * gl_WorkGroupSize.y);
    float dy = Re * dphi;

    vec4 N_im1   = imageLoad(stateOut, coord + ivec2(0, -1));
    vec4 N_ip1   = imageLoad(stateOut, coord + ivec2(0,  1));

    float X_i    = phi * Re;
    float X_ip1  = X_i + dy;
    float X_im1  = X_i - dy;

    float Xb_j   = X_i - (dy / 2);
    float Xb_jp1 = X_i + (dy / 2);

    float Wb_j   = cos(phi - (dphi / 2)) * float(coord.y > 0);
    float Wb_jp1 = cos(phi + (dphi / 2)) * float(coord.y < imgsize.y - 1);

    float W_i    = cos(phi);

    float K_j    = D / C * Re * Re * (1 + f);
    float K_jp1  = D / C * Re * Re * (1 + f);

    float U_j    = 0;
    float U_jp1  = 0;

    float S_i    = 0;

    float Tl = (Wb_j / W_i) * (K_j + U_j * (X_i - Xb_j)) / ((Xb_jp1 - Xb_j) * (X_i - X_im1));

    float Tm = 0;
    Tm -= (Wb_jp1 * (K_jp1 + U_jp1 * (X_ip1 - Xb_jp1))) / (W_i * (Xb_jp1 - Xb_j) * (X_ip1 - X_i  ));
    Tm -= (Wb_j   * (K_j   - U_j   * (Xb_j  - X_im1 ))) / (W_i * (Xb_jp1 - Xb_j) * (X_i   - X_im1));

    float Tu = (Wb_jp1 / W_i) * (K_jp1 - U_jp1 * (Xb_jp1 - X_i)) / ((Xb_jp1 - Xb_j) * (X_ip1 - X_i));

    return (Tl * N_im1 + Tm * N + Tu * N_ip1) + S_i;
}

float calc_clausius_clapeyron(float T) {
    float Tcel = T - 273.15;
    return 6.112 * exp(17.67 * Tcel / (Tcel + 243.5));
}

float calc_qsat(float T, float P) {
    // T in Kelvin
    // P in hPa or mb
    float es = calc_clausius_clapeyron(T);
    return eps * es / (P - (1 - eps) * es);
}

float calc_f(float T) {
    // could be an input later
    const float RH     = 0.8f;
    const float deltaT = 0.01;

    float dqsdTs = (calc_qsat(T + deltaT / 2.0f, 1000.0) - calc_qsat(T - deltaT / 2.0f, 1000.0)) / deltaT;

    return Lh_vap * RH * dqsdTs / gas_cp;
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

    // compute moist ampl factor
    float f = calc_f(value.r);

    // adv diff
    float dTdt = calc_merid_advdiff(value, texelCoord, lat, C_val, f).r;
    value.g = dTdt;
    value.r += dTdt * dt * secs_per_day;

    imageStore(stateOut, texelCoord, value);
}
