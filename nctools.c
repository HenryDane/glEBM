#include "nctools.h"
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int try_read_ncvar(int ncid, int prev_ret, const char* name, int* varid) {
    if (prev_ret == NC_NOERR) return prev_ret;

    int retval = nc_inq_varid(ncid, name, varid);
    return retval;
}

int abort_ncop(int retval) {
    printf("Error: %s\n", nc_strerror(retval));
    exit(2);
}

//void

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
