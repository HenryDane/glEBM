#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

//layout(rgba32f, binding = 0) uniform image2D stateIn;
//layout(rgba32f, binding = 1) uniform image2D stateOut;
layout(rgba32f, binding = 0) uniform image2D stateOut;
layout(binding = 1) uniform sampler2D insol_LUT;

layout(location = 0) uniform float t;
layout(location = 1) uniform float dt;

// physical constants
const float pi            =    3.14159265;
const float days_per_year =  365.0f;
const float secs_per_day  = 86400.0f;
const float S0            = 1367.0f;
const float C_val         = (4e3) * (1e3) * (10.0); // 10m of water
const float sigma         = 5.67e-8f;
const float Re            = 6.378e6f;
const float omega         = 7.292e-5;

// orbital parameters
const float ecc       =   0.01724f;
const float obliquity =  23.45f;
const float long_peri = 281.4f;

// albedo parameters
const float a0 =   0.3f;
const float a2 =   0.078f;
const float ai =   0.62f;
const float Tf = 263.15f;

// boltzmann parameters
const float bm_A = 210.0f;
const float bm_B =   2.0f;
const float tau  =   0.61f;

float deg2rad(float x) {
    return pi / 180.0f * x;
}

float calcP2(float x) {
    return 0.5 * (3 * x * x - 1.0);
}

float calc_Q(float lat, float lon, float day) {
    vec2 coord  = vec2(day / 365.0f, (lat + 90.0f) / 180.0f);
    vec4 soldat = texture(insol_LUT, coord); // solar_lon, S0*b2/a2, H0, delta

    float phi    = deg2rad(lat);
    float h      = (mod((mod(day, 1.0) + (lon / 360)), 1.0) - 0.5) * 2 * pi;
    float coszen = sin(phi)*sin(soldat.a) + cos(phi)*cos(soldat.a)*cos(h);
    float Fsw    = (abs(h) < soldat.b) ? soldat.g * coszen : 0.0f;

    return clamp(Fsw, 0.0f, S0);
}

float calc_albedo(float Ts, float lat) {
    float phi = deg2rad(lat);

    return (Ts > Tf) ? a0 + a2 * calcP2(sin(phi)) : ai;
}

float calc_Ts(float albedo, float Q, float T_old) {
    return (((1 - albedo) * Q) - (tau * sigma * T_old * T_old * T_old * T_old)) / C_val;
}

vec4 safeRead(ivec2 coord) {
    ivec2 imgsize = imageSize(stateOut);
    coord.y = clamp(coord.y, 0, imgsize.y - 1);
    ivec2 tc = ivec2(mod(coord + imgsize, imageSize(stateOut)));

    return imageLoad(stateOut, tc);
}

vec4 calc_adv_diff(vec4 S, ivec2 tcoord, float lat) {
    // define diffusivities for temp and humidity
    const vec4 Kxx = vec4(2e-2, 1.0, 0.0, 0.0);
    const vec4 Kyy = vec4(2e-2, 1.0, 0.0, 0.0);

    // calculate dx, dy from (4.1)
    float dlambda = 2.0 * pi / float(gl_NumWorkGroups.x);
    float dphi = pi / float(gl_NumWorkGroups.y);
    float dx = Re * cos(deg2rad(lat)) * dlambda;
    float dy = Re * dphi;

    vec4 Sipj = safeRead(tcoord + ivec2( 1,  0));
    vec4 Simj = safeRead(tcoord + ivec2(-1,  0));
    vec4 Sijp = safeRead(tcoord + ivec2( 0,  1));
    vec4 Sijm = safeRead(tcoord + ivec2( 0, -1));

    // compute dN/dt
//    vec4 dNdt = vec4(0);
//    vec4 dNdtAX = ((Sipj * Sipj.aaaa - Simj * Simj.aaaa) / (2.0 * dx));
//    vec4 dNdtAY = ((Sijp * Sijp.bbbb - Sijm * Sijm.bbbb) / (2.0 * dy));
    vec4 dNdtKX = ((Simj + Sipj - (2.0f * S)) * (Kxx / (dx * dx)));
    vec4 dNdtKY = ((Sijm + Sijp - (2.0f * S)) * (Kyy / (dy * dy)));
//    vec4 dNdt = - (dNdtAX + dNdtAY + dNdtKX + dNdtKY);
    vec4 dNdt = (dNdtKX + dNdtKY);

    return vec4(dNdt.r, dNdt.g, 0.0, 0.0);
}

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    // Ts, q, u, v
    //  r, g, b, a
    vec4 value = imageLoad(stateOut, texelCoord);

    // coordinates
    float day = t;
    float lat = (float(gl_GlobalInvocationID.y + 0.5) / float(gl_NumWorkGroups.y) * 180.0f) - 90.0f;
    float lon = (float(gl_GlobalInvocationID.x) / float(gl_NumWorkGroups.x) * 360.0f);

    // pressure
    // float Pa = NA * kB * value.r; // (2.22)

    // compute instant insolation
    float Q = calc_Q(lat, lon, day);

    // compute albedo
    float alpha = calc_albedo(value.r, lat);

    // compute temperature
    value.r = value.r + calc_Ts(alpha, Q, value.r) * (dt * secs_per_day);

    // calculate advdiff
    value.rg += (calc_adv_diff(value, texelCoord, lat).rg * dt * secs_per_day);

    imageStore(stateOut, texelCoord, value);
}
