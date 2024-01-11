#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "common.h"
#include "profile.h"
#include "initial.h"
#include "fetch.h"
#include "renderutil.h"
#include "nctools.h"

// https://learnopengl.com/Guest-Articles/2022/Compute-Shaders/Introduction
// https://medium.com/@daniel.coady/compute-shaders-in-opengl-4-3-d1c741998c03
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

const char* get_gl_err_type_str(GLenum type) {
    switch(type) {
    case GL_DEBUG_TYPE_ERROR:
        return "GL_ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "GL_DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "GL_UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        return "GL_PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "GL_PERFORMANCE";
    case GL_DEBUG_TYPE_OTHER:
        return "GL_INFO";
    }

    return "GL_UNKNOWN";
}

void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam){

    printf("[%s] type=%#02x severity=%#02x source=%#02x id=%#02x message: %s\n",
        get_gl_err_type_str(type), type, severity, source, id, message);

	if (type == GL_DEBUG_TYPE_ERROR) {
		exit(-20); // commenting this makes screen sharing possible i have no idea why
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

    // setup profiling data
    profile_t profldat;
    init_profile(&profldat);

    // load netcdf4 input file
    model_initial_t initial_model;
    size_t model_size_x, model_size_y;
    read_input(&model_size_x, &model_size_y,
        &initial_model.lats, &initial_model.lons,
        &initial_model.Ts, &initial_model.Bs, &initial_model.lambdas);

    model_storage_t model;
    init_model_storage(&model, 10.0f,
        model_size_x, model_size_y);

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

    // create physical LUT (lat, lon, B, lambda) texture
    unsigned int physp_LUT = make_LUT(model_size_x, model_size_y,
        &initial_model);

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
    unsigned int css_physp_LUT_l = glGetUniformLocation(compute_shader, "physp_LUT");

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
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, physp_LUT);

    float t = 0.0f; // in days
    float dt = model.timestep; // 5 mins

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
        glUniform1i(css_physp_LUT_l, 2);
        glDispatchCompute((unsigned int)model_size_x / 32, (unsigned int)model_size_y / 32, 1);
        t += dt;

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
            if (t <= days_per_year) {
                printf("fc=%d t=%.4f (days) dt=%.4f (mins) tps=%.2f\n", frame_ctr,
                    t, dt * 24.0f * 60.0f, 1.0f / delta);
            } else {
                printf("fc=%d t=%.4f (yr) dt=%.4f (mins) tps=%.2f\n", frame_ctr,
                    t / days_per_year, dt * 24.0f * 60.0f, 1.0f / delta);
            }
#endif // REDUCED_OUTPUT
            float* data = fetch_2d_state(
                surf_texture, model_size_x, model_size_y, &Tmax, &Tmin,
                &qmax, &qmin, &umax, &umin, &vmax, &vmin);
            model_storage_add_frame(&model, t, data);
        }

        // run for 8 years
        if (t > model.final_time) {
            const char* path = "output.nc";
            printf("Run complete.\nSaving results to %s...\n", path);
            // get the data agian
            float* data = fetch_2d_state(
                surf_texture, model_size_x, model_size_y, &Tmax, &Tmin,
                &qmax, &qmin, &umax, &umin, &vmax, &vmin);
            // add it to the pile
            model_storage_add_frame(&model, t, data);
            // write it to disk
            model_storage_write(model_size_x, model_size_y, &model, &initial_model, path);
            // delete it
            model_storage_free(&model);
            // stop the run
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }

        // frame counter
        frame_ctr++;
    }

    glfwTerminate();
}
