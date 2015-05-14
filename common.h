#pragma once
#include "config.h"
#ifdef GPSD_FOUND
#   include <gps.h>
#endif

#define LEN_QUERY  1024
#define LEN        256
#define LEN_BUFFER 1024

// the engine data struct
typedef struct obd_data_t {
    float   rpm;
    float   injection_time;
    float   oil_pressure;
    float   speed;

    // consumption (will be calculated)
    float   consumption_per_h;
    float   consumption_per_100km;

    float   duration_consumption;
    float   duration_speed;

    float   temp_engine;
    float   temp_air_intake;
    float   voltage;

#ifdef GPSD_FOUND
    struct gps_fix_t gps;
#endif

} obd_data_t;
