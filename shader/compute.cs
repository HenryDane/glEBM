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
const float h2o_c0     =   4.0e6;
const float ice_c0     =   1.919e6;
const float depth      =  50.0;
const float ice_Li     =   3.0e8;
const float ice_Tm     = 273.15;
const float ice_alpha  =   0.56;
const float ice_deltaa =   0.48;
const float ice_ha     =   0.5;
const float ice_c0H0   = h2o_c0 * depth;

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

vec4 calc_merid_advdiff(vec4 N, ivec2 coord, float lat, float K_j, float K_jp1) {
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

    // coordinate system conversion
    K_j         *= Re * Re;
    K_jp1       *= Re * Re;

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

float calc_K(vec4 value) {
    // thermal conductivity
    const float D_ice = 2.22;
    const float D_h2o = 0.555;

    // calculate h_ice
    float h_ice = -value.g / ice_Li * float(value.g < 0);

    // calc effective specific heat
    float C = 0;
    C += depth * ice_c0 * float(h_ice >  0);
    C += depth * h2o_c0 * float(h_ice <= 0);

    // calculate effective thermal conductivity
    float D = 0;
    D += D_ice * float(h_ice >  0);
    D += D_h2o * float(h_ice <= 0);

    // calculate K values
    // only return K != 0 iif the cell contains liquid water
    // since I don't think it makes sense to diffuse the ice itself
    // but I'm not sure...
    return D / C * float(value.g > 0);
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

    // compute K values
    float K_p   = calc_K(imageLoad(stateOut, texelCoord + ivec2(0,  1)));
    float K_m   = calc_K(imageLoad(stateOut, texelCoord + ivec2(0, -1)));
    float K_i   = calc_K(value);
    float K_j   = 0.5 * (K_i + K_m);
    float K_jp1 = 0.5 * (K_i + K_p);

    // advect E
    float dEdt_diff = calc_merid_advdiff(value, texelCoord, lat, K_j, K_jp1).g;
    value.g += dEdt_diff * dt * secs_per_day;
    value.a = K_j;

    // compute change in E
    float ASR  = calc_ASR(alpha, Q);
    float corr = ice_alpha + (ice_deltaa / 2) * tanh(value.g / (ice_Li * ice_ha));
    ASR       *= corr;
    float OLR  = calc_OLR(value.r, olr_A);
    float dEdt = ASR - OLR + ice_FB;
    value.g   += dEdt * dt * secs_per_day;

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
