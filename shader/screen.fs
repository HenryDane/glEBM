#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;

const float S0            = 1367.0f;

void main() {
    vec3 value = texture(tex, TexCoords).rgb;

    value.r = value.r / S0;
    value.g = (value.g - 0.26) / (0.38 - 0.26);
    value.b = (value.b - 0.0) / 80;

    FragColor = vec4(vec3(value.b), 1.0);
}
