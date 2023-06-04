#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;
uniform float Tmax;
uniform float Tmin;

const float S0 = 1367.0f;

void main() {
    vec3 value = texture(tex, TexCoords).rgb;

    value.r = value.r / S0;
//    value.g = (value.g - 0.26) / (0.38 - 0.26);
//    value.b = (value.b - 50) / (200 - 50);
//    value.b = float(value.b > 263.15);
    value.b = (value.b - Tmin) / (Tmax - Tmin);

//    FragColor = vec4(vec3(value.b), 1.0);
    FragColor = vec4(vec3(value.r), 1.0);
//    FragColor = vec4(value, 1.0);
}
