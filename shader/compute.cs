#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D imgOutput;

layout(location = 0) uniform float t;

const float pi            =    3.14159265;
const float days_per_year =  365.0f;
const float S0            = 1367.0f;

float deg2rad(float x) {
    return pi / 180.0f * x;
}

float solar_lon(float ecc, float long_peri_rad, float day) {
    float delta_lambda = (day - 80.) * 2*pi/days_per_year;
    float ecc2 = ecc * ecc;
    float ecc3 = ecc2 * ecc;
    float beta = sqrt(1 - ecc2);
    float lambda_long_m = -2*((ecc/2 + (ecc3)/8 ) * (1+beta) * sin(-long_peri_rad) -
        (ecc2)/4 * (1/2 + beta) * sin(-2*long_peri_rad) + (ecc3)/8 *
        (1/3 + beta) * sin(-3*long_peri_rad)) + delta_lambda;
    float lambda_long = ( lambda_long_m + (2*ecc - (ecc3)/4)*sin(lambda_long_m - long_peri_rad) +
        (5/4)*(ecc2) * sin(2*(lambda_long_m - long_peri_rad)) + (13/12)*(ecc3)
        * sin(3*(lambda_long_m - long_peri_rad)) );
    return lambda_long;
}

float instant_insol(float lat, float lon, float day, float ecc, float obliquity, float lambda_long, float long_peri) {
    float phi = deg2rad(lat);
    float delta = asin(sin(deg2rad(obliquity)) * sin(lambda_long));
    float h = (mod((mod(day, 1.0) + (lon / 360)), 1.0) - 0.5) * 2 * pi;
    float Ho = abs(delta)-pi/2+abs(phi) < 0 ? acos(-tan(phi)*tan(delta)) : (phi*delta>0. ? pi : 0.0f);
    float coszen = sin(phi)*sin(delta) + cos(phi)*cos(delta)*cos(h);
    float a1 = (1-ecc*ecc);
    float a2 = a1 * a1;
    float b1 = (1+ecc*cos(lambda_long - deg2rad(long_peri)));
    float b2 = b1 * b1;
    float Fsw = (abs(h) < Ho) ? S0*( b2 / a2 * coszen) : 0.0f;

    return Fsw;
}

void main() {
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

    float day = t;
    float lat = (float(gl_GlobalInvocationID.y) / gl_NumWorkGroups.y * 180.0f) - 90.0f;
    float lon = (float(gl_GlobalInvocationID.x) / gl_NumWorkGroups.x * 360.0f);

    float ecc = 0.01724f;
    float obliquity = 23.45f;
    float long_peri = 281.4f;
    float lambda_long = solar_lon(ecc, deg2rad(long_peri), day);

    value.rgb = vec3(instant_insol(lat, lon, day, ecc, obliquity, lambda_long, long_peri) / S0);
    imageStore(imgOutput, texelCoord, value);
}
