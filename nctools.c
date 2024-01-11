#include "nctools.h"
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <math.h>
#include <string.h>

int try_read_ncvar(int ncid, int prev_ret, const char* name, int* varid) {
    if (prev_ret == NC_NOERR) return prev_ret;

    int retval = nc_inq_varid(ncid, name, varid);
    return retval;
}

void abort_ncop(int retval) {
    printf("Error: %s\n", nc_strerror(retval));
    exit(2);
}

void check_retval(int retval) {
    if (retval != NC_NOERR) {
        abort_ncop(retval);
    }
}

void init_model_storage(model_storage_t* model, float final_time,
    int model_width, int model_height) {
    model->timestep = (1.0f / 24.0f) * (5.0f / 60.0f); // in days
    model->final_time = final_time;
    model->n_timesteps = (int) ceilf(final_time / model->timestep);
    model->n_slots = (model->n_timesteps / 500) + 1;

    // create head
    model->head = malloc(sizeof(storage_frame_t));
    model->head->next = NULL;
    model->head->prev = NULL;
    model->head->time = 0.0f;
    model->head->data = NULL;
}

void model_storage_add_frame(model_storage_t* model, float time, float* data) {
    // create a frame
    storage_frame_t* frame = (storage_frame_t*) malloc(sizeof(storage_frame_t));
    frame->prev = model->head;
    frame->next = NULL;
    frame->time = time;
    frame->data = data;

    // update head
    model->head->next = frame;

    // update model
    model->head = frame;
}

void model_storage_write(int size_x, int size_y, model_storage_t* model,
    model_initial_t* initial, const char* path) {
    // find first node
    int num = 0;
    while (model->head->prev != NULL) {
        model->head = model->head->prev;
        num++;
    }

    // head points to first node, check that t==0.0f, data==NULL
    if (model->head->time > 0.001 || model->head->data != NULL) {
        // something is wrong
        printf("Model storage is mangled!\n");
        exit(3);
    }

    // temporary values and ids
    int retval;
    int ncid, lat_dimid, lat_varid, lon_dimid, lon_varid;
    int time_dimid, time_varid;
    int Ti_varid, Bp_varid, Ts_varid;

    // string names
    const char* lat_name  = "lat";
    const char* lon_name  = "lon";
    const char* time_name = "time";
    const char* Ti_name   = "T_initial";
    const char* Ts_name   = "Ts";
    const char* Bp_name   = "param_B";
    const char* units_str = "units";
    const char* deg_north = "degrees_north";
    const char* deg_east  = "degrees_east";
    const char* units_K   = "kelvin";
    const char* units_B   = "w/m2/K";
    const char* units_day = "days";

    // create a nc file
    retval = nc_create(path, NC_CLOBBER, &ncid);
    check_retval(retval);

    // make lat dimension
    retval = nc_def_dim(ncid, lat_name, size_y, &lat_dimid);
    check_retval(retval);

    // make lon dimension
    retval = nc_def_dim(ncid, lon_name, size_x, &lon_dimid);
    check_retval(retval);

    // make time dimension
    retval = nc_def_dim(ncid, time_name, NC_UNLIMITED, &time_dimid);
    check_retval(retval);

    // make lat coord var
    retval = nc_def_var(ncid, lat_name, NC_FLOAT, 1, &lat_dimid, &lat_varid);
    check_retval(retval);

    // make lon coord var
    retval = nc_def_var(ncid, lon_name, NC_FLOAT, 1, &lon_dimid, &lon_varid);
    check_retval(retval);

    // make time coord var
    retval = nc_def_var(ncid, time_name, NC_FLOAT, 1, &time_dimid, &time_varid);
    check_retval(retval);

    // define units for lat coord var
    retval = nc_put_att_text(ncid, lat_varid, units_str,
        strlen(deg_north), deg_north);
    check_retval(retval);

    // define units for lon coord var
    retval = nc_put_att_text(ncid, lon_varid, units_str,
        strlen(deg_east), deg_east);
    check_retval(retval);

    // define units for time coord var
//    retval = nc_put_att_text(ncid, time_varid, units_str,
//        strlen(units_day), units_day);
//    check_retval(retval);

    // prepare dimids arrays
    int dimid_2d[] = {lat_dimid, lon_dimid};
    int dimid_3d[] = {time_dimid, lat_dimid, lon_dimid};

    // define T_initial variable
    retval = nc_def_var(ncid, Ti_name, NC_FLOAT, 2, dimid_2d, &Ti_varid);
    check_retval(retval);

    // set T_initial units
    retval = nc_put_att_text(ncid, Ti_varid, units_str, strlen(units_K), units_K);
    check_retval(retval);

    // define param_B variable
    retval = nc_def_var(ncid, Bp_name, NC_FLOAT, 2, dimid_2d, &Bp_varid);
    check_retval(retval);

    // set param_B units
    retval = nc_put_att_text(ncid, Bp_varid, units_str, strlen(units_B), units_B);
    check_retval(retval);

    // define Ts variable
    retval = nc_def_var(ncid, Ts_name, NC_FLOAT, 3, dimid_3d, &Ts_varid);
    check_retval(retval);

    // set Ts units
    retval = nc_put_att_text(ncid, Ts_varid, units_str, strlen(units_K), units_K);
    check_retval(retval);

    // end define mode
    retval = nc_enddef(ncid);
    check_retval(retval);

    // write lats
    retval = nc_put_var_float(ncid, lat_varid, initial->lats);
    check_retval(retval);

    // write lons
    retval = nc_put_var_float(ncid, lon_varid, initial->lons);
    check_retval(retval);

    // write T_initial
    retval = nc_put_var_float(ncid, Ti_varid, initial->Ts);
    check_retval(retval);

    // write param_B
    retval = nc_put_var_float(ncid, Bp_varid, initial->Bs);
    check_retval(retval);

    // write Ts frame by frame
    size_t counts[] = {1, size_y, size_x};
    size_t starts[] = {0, 0, 0};
    float* Tdata = malloc(size_y * size_x * sizeof(float));
    while (model->head->next != NULL) {
        model->head = model->head->next;

        printf("Writing frame t=%f data=%x\n",
            model->head->time, model->head->data);

        // copy temperature data from model output
        for (size_t i = 0; i < size_y * size_x; i++) {
            Tdata[i] = model->head->data[(i * 4) + 0];
        }

        // write Ts
        retval = nc_put_vara_float(ncid, Ts_varid, starts, counts, Tdata);
        check_retval(retval);

        // write time
        retval = nc_put_var1_float(ncid, time_varid, &starts[0], &model->head->time);
        check_retval(retval);

        starts[0]++;
    }
    free(Tdata);

    // close file
    retval = nc_close(ncid);
    check_retval(retval);

    printf("Finished writing to %s.\n", path);
}

void model_storage_free(model_storage_t* model) {
    // find first node
    int num = 0;
    while (model->head->prev != NULL) {
        model->head = model->head->prev;
        num++;
    }

    // head points to first node, check that t==0.0f, data==NULL
    if (model->head->time > 0.001 || model->head->data != NULL) {
        // something is wrong
        printf("Model storage is mangled!\n");
        exit(3);
    }
}

void read_input(size_t* model_width, size_t* model_height,
    float** lats, float** lons,
    float** Ts_initial, float** Bs_initial, float** lambdas_initial) {
    int retval; // temporary for nc queries
    int ncid, lat_varid, lon_varid, lat_dimid, lon_dimid;
    int Ts_varid, Bs_varid, lambdas_varid;

//    const char* file_name = "cmip6-ensemble-mean-feedback.nc";
    const char* file_name = "input.nc";
    printf("Reading input file: %s\n", file_name);

    // open the file
    if ((retval = nc_open(file_name, NC_NOWRITE, &ncid))) {
       abort_ncop(retval);
    }

    // get varids of lat and lon. if either is missing, exit the program
    const char* lat_name = "lat";
    if ((retval = nc_inq_dimid(ncid, lat_name, &lat_dimid))) {
       abort_ncop(retval);
    }
    if ((retval = nc_inq_varid(ncid, lat_name, &lat_varid))) {
        abort_ncop(retval);
    }
    const char* lon_name = "lon";
    if ((retval = nc_inq_dimid(ncid, lon_name, &lon_dimid))) {
       abort_ncop(retval);
    }
    if ((retval = nc_inq_varid(ncid, lon_name, &lon_varid))) {
        abort_ncop(retval);
    }

    // get model width and height and check that they are multiple of 32
    retval = nc_inq_dimlen(ncid, lat_dimid, model_height);
    if (retval != NC_NOERR) {
        abort_ncop(retval);
    }
    if ((*model_height) % 32 != 0) {
        printf("Error: lat (model_height) must be a multiple of 32\n");
    }
    if ((*model_height) <= 0) {
        printf("Error: lat (model_height) must be greater than zero.\n");
        exit(3);
    }
    retval = nc_inq_dimlen(ncid, lon_dimid, model_width);
    if (retval) {
        abort_ncop(retval);
    }
    if ((*model_width) % 32 != 0) {
        printf("Error: lon (model_width) must be a multiple of 32\n");
        exit(3);
    }
    if ((*model_width) <= 0) {
        printf("Error: lon (model_width) must be greater than zero.\n");
        exit(3);
    }
    printf("Model size: (n_lon=%lu, n_lat=%lu)\n", *model_width, *model_height);

    // get lats
    *lats = (float*) malloc((*model_height) * sizeof(float));
    if (retval = nc_get_var_float(ncid, lat_varid, *lats)) {
        abort_ncop(retval);
    }

    // get lons
    *lons = (float*) malloc((*model_width) * sizeof(float));
    if (retval = nc_get_var_float(ncid, lon_varid, *lons)) {
        abort_ncop(retval);
    }

    // allocate memory for temperature
    *Ts_initial = (float*) malloc(
        (*model_width) * (*model_height) * sizeof(float));

    // check for temperature (could be named many diff things)
    retval = try_read_ncvar(ncid, NC_ENOTVAR, "T", &Ts_varid);
    retval = try_read_ncvar(ncid, retval, "Ts", &Ts_varid);
    retval = try_read_ncvar(ncid, retval, "Temperature", &Ts_varid);
    retval = try_read_ncvar(ncid, retval, "temperature", &Ts_varid);
    if (retval != NC_NOERR) {
        printf("Temperature data not found, using fallback.\n");
        // generate a replacement
        for (int i = 0; i < (*model_width) * (*model_height); i++) {
            (*Ts_initial)[i] = 273.15;
        }
    } else {
        printf("Found temperature data.\n");
        nc_get_var_float(ncid, Ts_varid, *Ts_initial);
    }

    // allocate memory for B parameter
    *Bs_initial = (float*) malloc(
        (*model_width) * (*model_height) * sizeof(float));

    // check for B parameter (could be named many diff things)
    retval = try_read_ncvar(ncid, NC_ENOTVAR, "B", &Bs_varid);
    retval = try_read_ncvar(ncid, retval, "Bs", &Bs_varid);
    if (retval != NC_NOERR) {
        printf("B parameter data not found, using fallback.\n");
        // generate a replacement
        for (int i = 0; i < (*model_width) * (*model_height); i++) {
            (*Bs_initial)[i] = 2.0f;
        }
    } else {
        printf("Found B parameter data.\n");
        nc_get_var_float(ncid, Bs_varid, *Bs_initial);
    }

    // allocate memory for lambda
    *lambdas_initial = (float*) malloc(
        (*model_width) * (*model_height) * sizeof(float));

    // check for lambda (could be named many diff things)
    retval = try_read_ncvar(ncid, NC_ENOTVAR, "lambda", &lambdas_varid);
    retval = try_read_ncvar(ncid, retval, "lambdas", &lambdas_varid);
    retval = try_read_ncvar(ncid, retval, "netFeedback", &lambdas_varid);
    if (retval != NC_NOERR) {
        printf("lambda data not found, using fallback.\n");
        // generate a replacement
        for (int i = 0; i < (*model_width) * (*model_height); i++) {
            (*lambdas_initial)[i] = 0.0f;
        }
    } else {
        printf("Found lambda data.\n");
        nc_get_var_float(ncid, lambdas_varid, *lambdas_initial);
    }
}
