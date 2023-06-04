#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "profile.h"
#include "initial.h"
#include "fetch.h"
#include "renderutil.h"

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

    profile_t profldat;
    init_profile(&profldat);

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

    // create 2d state texture
    float* data = make_2d_initial(SCR_WIDTH, SCR_HEIGHT);
    unsigned int surf_texture;
    glGenTextures(1, &surf_texture);
    glBindTexture(GL_TEXTURE_2D, surf_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, data);
    glBindImageTexture(0, surf_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, surf_texture);
//    glBindTexture(0);

    // create solar LUT texture
    unsigned int solat_LUT = make_solar_table();

    // make shaders
    unsigned int compute_shader = create_cshader("shader/compute.cs");
    unsigned int screen_shader  = create_shader("shader/screen.vs", "shader/screen.fs");

    // configure screen shader
    glUseProgram(screen_shader);
    glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
    unsigned int ss_tmax_l = glGetUniformLocation(screen_shader, "Tmax");
    unsigned int ss_tmin_l = glGetUniformLocation(screen_shader, "Tmin");

    // figure out compute shader stuff
    unsigned int css_t_l         = glGetUniformLocation(compute_shader, "t");
    unsigned int css_dt_l        = glGetUniformLocation(compute_shader, "dt");
    unsigned int css_insol_LUT_l = glGetUniformLocation(compute_shader, "insol_LUT");

    // timing state info
    float currentFrame, delta, tlast = 0.0f;
    int frame_ctr = 0;
    float speed = 1.0f; // 1s -> 1 day

    // state info
    float Tmin = 200.0f;
    float Tmax = 300.0f;

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, surf_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, solat_LUT);

    // process window/graphics
    while (!glfwWindowShouldClose(window)) {
        // compute frame time
        float currentFrame = glfwGetTime();
        delta = currentFrame - tlast;
        tlast = currentFrame;

        // dispatch compute shader
        glUseProgram(compute_shader);
        glUniform1f(css_t_l, currentFrame * speed);
        glUniform1f(css_dt_l, delta * speed);
        glUniform1i(css_insol_LUT_l, 1);
        glDispatchCompute((unsigned int)SCR_WIDTH, (unsigned int)SCR_HEIGHT, 1);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(screen_shader);
        glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
        glUniform1f(ss_tmax_l, Tmax);
        glUniform1f(ss_tmin_l, Tmin);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, surf_texture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // finish frame
        glfwSwapBuffers(window);
        glfwPollEvents();

        // update profiling
        tick_profile(&profldat, delta, frame_ctr);

        // collect statistics
        if (frame_ctr % 500 == 0) {
#ifndef REDUCED_OUTPUT
            printf("fc=%d t=%.4edays \n", frame_ctr, currentFrame * speed);
#else
            printf("%.4e ", currentFrame * speed);
#endif // REDUCED_OUTPUT
            fetch_2d_state(surf_texture, SCR_WIDTH, SCR_HEIGHT, &Tmax, &Tmin);
        }

        if (frame_ctr > 10125) break;

        // frame counter
        frame_ctr++;
    }

    glfwTerminate();
}
