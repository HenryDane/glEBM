#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;
uniform vec4 maxs;
uniform vec4 mins;

const float S0 = 1367.0f;

// https://observablehq.com/@flimsyhat/webgl-color-maps
vec3 color_map(float t) {
    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}

void main() {
    vec4 value = texture(tex, TexCoords).rgba;

    value = (value - mins) / (maxs - mins);

    FragColor = vec4(color_map(clamp(value.r, 0, 1)), 1.0);
//    FragColor = vec4(vec3(value.r), 1.0);
//    FragColor = vec4(vec3(value.a), 1.0);
//    FragColor = vec4(1 - value.a, 0.0, 0.0, 1.0);
//    FragColor = vec4(1 - value.b, 0.0, 0.0, 1.0);
//    FragColor = vec4(vec3(value.b, value.a, 0.0), 1.0);
//    FragColor = (isnan(value.a) ? vec4(1.0, 0.0, 0.0, 0.0) : FragColor);
//    FragColor = vec4(value.r / 39135.0f, value.g / 39135.0f, 0.0f, 1.0);
}
