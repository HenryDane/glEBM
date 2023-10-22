#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

bool is_pow2(int x) {
    return (x > 0) && ((x & (x - 1)) == 0);
}

int main(int argc, char *argv[]) {
    int model_size_x = DEFAULT_MODEL_WIDTH;
    int model_size_y = DEFAULT_MODEL_HEIGHT;

    // parse arguments
    if (argc > 1){
        char* p;
        // require either all arguments specified, or none specified
        if (argc != 3) {
            printf("Usage: glEBM width height\n");
            return 0;
        }

        model_size_x = strtol(argv[1], &p, 10);
        if (errno == ERANGE) {
            printf("Error: width value could not be converted to int.\n");
            return 0;
        }
        if (!is_pow2(model_size_x)) {
            printf("Error: width value must be a power of 2.\n");
            return 0;
        }

        model_size_y = strtol(argv[2], &p, 10);
        if (errno == ERANGE) {
            printf("Error: height value could not be converted to int.\n");
            return 0;
        }
        if (!is_pow2(model_size_y)) {
            printf("Error: height value must be a power of 2.\n");
            return 0;
        }
    }

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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
    printf("Max invocations: %d\n", max_compute_work_group_invocations);
    printf("Model size: %d %d (%d %d)\n", model_size_x, model_size_y, model_size_x / 32, model_size_y / 32);

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
    float* data = make_2d_initial(model_size_x, model_size_y);
    unsigned int surf_texture;
    glGenTextures(1, &surf_texture);
    glBindTexture(GL_TEXTURE_2D, surf_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, model_size_x, model_size_y, 0,
        GL_RGBA, GL_FLOAT, data);
    glBindImageTexture(0, surf_texture, 0, GL_FALSE, 0, GL_READ_WRITE,
        GL_RGBA32F);

    // create solar LUT texture
    unsigned int solat_LUT = make_solar_table();

    // make shaders
    unsigned int compute_shader = create_cshader("shader/compute.cs");
    unsigned int screen_shader  = create_shader("shader/screen.vs", "shader/screen.fs");

    // configure screen shader
    glUseProgram(screen_shader);
    glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
    unsigned int ss_maxs_l = glGetUniformLocation(screen_shader, "maxs");
    unsigned int ss_mins_l = glGetUniformLocation(screen_shader, "mins");

    // figure out compute shader stuff
    unsigned int css_t_l         = glGetUniformLocation(compute_shader, "t");
    unsigned int css_dt_l        = glGetUniformLocation(compute_shader, "dt");
    unsigned int css_insol_LUT_l = glGetUniformLocation(compute_shader, "insol_LUT");

    // timing state info
    float currentFrame, delta, tlast = 0.0f;
    int frame_ctr = 0;

    // state info
    float Tmin =  1e9; float qmin =  1e9; float umin =  1e9; float vmin =  1e9;
    float Tmax = 1e-9; float qmax = -1e9; float umax = -1e9; float vmax = -1e9;

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, surf_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, solat_LUT);

    float t = 0.0f;
//    float dt = 1e-1f;
    float dt = (1.0f / 24.0f) * (1 / 12.0f); // 5 mins

    // process window/graphics
    while (!glfwWindowShouldClose(window)) {
        // compute frame time
        float currentFrame = glfwGetTime();
        delta = currentFrame - tlast;
        tlast = currentFrame;

        // dispatch compute shader
        glUseProgram(compute_shader);
        glUniform1f(css_t_l, t);
        glUniform1f(css_dt_l, dt);
        glUniform1i(css_insol_LUT_l, 1);
        glDispatchCompute((unsigned int)model_size_x / 32, (unsigned int)model_size_y / 32, 1);
        t += dt;

        // make sure writing to image has finished before read
//        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(screen_shader);
        glUniform1i(glGetUniformLocation(screen_shader, "tex"), 0);
        glUniform4f(ss_maxs_l, Tmax, qmax, umax, vmax);
        glUniform4f(ss_mins_l, Tmin, qmin, umin, vmin);
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
//            printf("fc=%d t=%.4f (days) dt=%.4f (mins) tps=%.2f\n", frame_ctr,
//                currentFrame * speed, delta * speed * 24.0f * 60.0f, 1.0f / delta);
            if (t <= days_per_year) {
                printf("fc=%d t=%.4f (days) dt=%.4f (mins) tps=%.2f\n", frame_ctr,
                    t, dt * 24.0f * 60.0f, 1.0f / delta);
            } else {
                printf("fc=%d t=%.4f (yr) dt=%.4f (mins) tps=%.2f\n", frame_ctr,
                    t / days_per_year, dt * 24.0f * 60.0f, 1.0f / delta);
            }
#else
            printf("%.4e ", currentFrame * speed);
#endif // REDUCED_OUTPUT
            fetch_2d_state(surf_texture, model_size_x, model_size_y, &Tmax, &Tmin,
                &qmax, &qmin, &umax, &umin, &vmax, &vmin);
        }

        if (t > days_per_year * 3.0) {
            const char* path = "result.csv";
            fetch_and_dump_state(surf_texture, model_size_x, model_size_y, path);
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }

        // frame counter
        frame_ctr++;
    }

    glfwTerminate();
}
