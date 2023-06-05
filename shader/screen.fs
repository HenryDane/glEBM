#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;
uniform vec4 maxs;
uniform vec4 mins;

const float S0 = 1367.0f;

void main() {
    vec4 value = texture(tex, TexCoords).rgba;

    value = (value - mins) / (maxs - mins);

//    FragColor = vec4(vec3(value.r), 1.0);
    FragColor = vec4(vec3(value.a), 1.0);
    FragColor = (isnan(value.a) ? vec4(1.0, 0.0, 0.0, 0.0) : FragColor);
//    FragColor = vec4(value.r / 39135.0f, value.g / 39135.0f, 0.0f, 1.0);
}
