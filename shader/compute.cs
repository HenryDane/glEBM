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
const float a0 =   0.3f;
const float a2 =   0.078f;
const float ai =   0.62f;
const float Tf = 263.15f;

// OLR parameters
const float olr_A = 210.0f;
const float olr_B =   2.0f;

// ice parameters
const float ice_FB     =   0;
const float ice_B      =   2.83;
const float ice_ki     =   1.981;
const float ice_c0H0   =   2.0e8;
const float ice_Li     =   3.0e8;
const float ice_Tm     = 273.15;
const float ice_alpha  =   0.56;
const float ice_deltaa =   0.48;
const float ice_ha     =   0.5;

float deg2rad(float x) {
    return pi / 180.0f * x;
}

float calcP2(float x) {
    return 0.5 * (3 * x * x - 1.0);
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
    float is_freezing = float(Tf > Ts);
    float albedo = 0;
    albedo += is_freezing * ai;
    albedo += (1 - is_freezing) * (a0 + a2 * calcP2(phi));
    return albedo;
}

float calc_ASR(float albedo, float Q) {
    float ASR = (1 - albedo) * Q;
    return ASR;
}

float calc_OLR(float T, float A) {
    return A + (olr_B * (T - 273.15));
}

vec4 calc_merid_advdiff(vec4 N, ivec2 coord, float lat, float C) {
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

    float K_j    = D / C * Re * Re;
    float K_jp1  = D / C * Re * Re;

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

void main() {
    // fetch data from state texture
    // Ts, E, h_ice, A
    //  r, g,     b, a
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec4 value = imageLoad(stateOut, texelCoord);

    // coordinates
    float day = t; //mod(t, days_per_year);
    float lat = (float(gl_GlobalInvocationID.y + 0.5) / float(gl_NumWorkGroups.y * gl_WorkGroupSize.y) * 180.0f) - 90.0f;
    float lon = (float(gl_GlobalInvocationID.x + 0.5) / float(gl_NumWorkGroups.x * gl_WorkGroupSize.x) * 360.0f);

    // compute instant insolation
    float Q = calc_Q(lat, lon, day);

    // compute albedo
    float alpha = calc_albedo(value.r, lat);

    // compute change in E
    float ASR  = calc_ASR(alpha, Q);
    float corr = ice_alpha + (ice_deltaa / 2) * tanh(value.g / (ice_Li * ice_ha));
    ASR       *= corr;
    float OLR  = calc_OLR(value.r, olr_A);
    float dEdt = ASR - OLR + ice_FB;
    value.g   += dEdt;

    // compute h_ice
    float h_ice = -value.g / ice_Li * float(value.g < 0);
    value.b     = h_ice;

    // compute A from fluxes
    // use (3) and old temperature
    float A = dEdt + (ice_B * (value.r - 273.15)) - ice_FB;
    value.a = A;

    // compute temperature (C)
    float T = 0;
    T += (value.g / ice_c0H0) * float(value.g >= 0);
    T += (ice_Tm - 273.15) * (float(value.g < 0) * float(A > 0));
    T += (A / ice_B) * (1 / (1 + ice_ki / (ice_B * h_ice))) * (float(value.g < 0) * float(A < 0));

    // convert temperature
    T += 273.15;

    // store temperature
    value.r = T;

    imageStore(stateOut, texelCoord, value);
}
