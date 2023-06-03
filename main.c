#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"

// https://learnopengl.com/Guest-Articles/2022/Compute-Shaders/Introduction
// https://medium.com/@daniel.coady/compute-shaders-in-opengl-4-3-d1c741998c03
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam){
    printf("%s type=0x%04X severity=0x%04X %s\n", (type == GL_DEBUG_TYPE_ERROR ? "*** GL ERROR ***" : "*** GL INFO ***" ),
    type, severity, message);

    if (type == GL_DEBUG_TYPE_ERROR) {
        exit(1); // commenting this makes screen sharing possible i have no idea why
    }
}

int main() {
    // register an error callback
    glfwSetErrorCallback(error_callback);

    // initialize glfw
    if (!glfwInit()) {
        printf("Unable to initialize GLFW. Exiting...\n");
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    printf("Loaded GLFW: %s\n", glfwGetVersionString());

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "glGCM", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    // initialize glew
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        exit(1);
    }
    printf("Loaded GLEW: %s\n", glewGetString(GLEW_VERSION));

    // setup GL
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(messageCallback, 0);

    // query limitations
    int max_compute_work_group_count[3];
    int max_compute_work_group_size[3];
    int max_compute_work_group_invocations;
    for (int idx = 0; idx < 3; idx++) {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
    }
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);

    // display query results
    printf("Max work groups: %d %d %d\n", max_compute_work_group_count[0],
        max_compute_work_group_count[1], max_compute_work_group_count[2]);
    printf("Max group size: %d %d %d\n", max_compute_work_group_size[0],
        max_compute_work_group_size[1], max_compute_work_group_size[2]);

    // create quad
    unsigned int quadVBO, quadVAO = 0;
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // create 2d texture
    unsigned int state_texture;
    glGenTextures(1, &state_texture);
    glBindTexture(GL_TEXTURE_2D, state_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, state_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state_texture);

    // make shaders
    unsigned int compute_shader = create_cshader("shader/compute.cs");
    unsigned int screen_shader  = create_shader("shader/screen.vs", "shader/screen.fs");

    // configure screen shader
    glUseProgram(screen_shader);
    glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);

    // figure out compute shader stuff
    unsigned int css_t_l = glGetUniformLocation(compute_shader, "t");

    // timing stuff
    const int N_TIMING_SAMPLES = 1000;
    int profiling_state = 0;
    float dts[N_TIMING_SAMPLES];
    for (size_t i = 0; i < N_TIMING_SAMPLES; dts[i++] = 0.0f);

    // process window/graphics
    float currentFrame, delta, tlast = 0.0f;
    int frame_ctr = 0;
    while (!glfwWindowShouldClose(window)) {
        // compute frame time
        float currentFrame = glfwGetTime();
        delta = currentFrame - tlast;
        tlast = currentFrame;

        if (profiling_state >= 0 && profiling_state < N_TIMING_SAMPLES && frame_ctr > 120) {
            dts[profiling_state] = delta;
            profiling_state++;

            if (profiling_state >= N_TIMING_SAMPLES) {
                profiling_state = -1;
            }
        }

        // dispatch compute shader
        glUseProgram(compute_shader);
        glUniform1f(css_t_l, currentFrame);
        glDispatchCompute((unsigned int)SCR_WIDTH, (unsigned int)SCR_HEIGHT, 1);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(screen_shader);
        glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, state_texture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // finish frame
        glfwSwapBuffers(window);
        glfwPollEvents();

        // frame time
        if (profiling_state == -1) {
            profiling_state = -2;

            float max = 0.0f;
            float min = 10000.0f;
            float mean = 0.0f;
            for (size_t i = 0; i < N_TIMING_SAMPLES; i++) {
                if (dts[i] < min) {
                    min = dts[i];
                }
                if (dts[i] > max) {
                    max = dts[i];
                }
                mean += dts[i];
            }
            mean = mean / ((float) N_TIMING_SAMPLES) * 1000.0f;
            min *= 1000.0f;
            max *= 1000.0f;

            printf("mean=%.3f min=%.3f max=%.3f\n", mean, min, max);
        }

        frame_ctr++;
    }

    glfwTerminate();
}
