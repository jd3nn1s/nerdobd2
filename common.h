#pragma once
#include "config.h"
#include <stdint.h>
#ifdef GPSD_FOUND
#   include <gps.h>
#endif

#define LEN_QUERY  1024
#define LEN        256
#define LEN_BUFFER 1024

// the engine data struct
typedef struct obd_data_t {
    float   rpm;
    float   oil_pressure;
    float   speed;

    float   fuel_remaining;
    uint8_t fuel_level;

    float   temp_oil;
    float   temp_coolant;
    float   temp_air_intake;
    float   voltage;

    double latitude;
    double longitude;
    float  altitude;
    float  track;
    float  gps_speed;

    uint8_t throttle_angle;
} __attribute__((__packed__)) obd_data_t;

#define FIELD_THROTTLE_ANGLE 1
#define FIELD_INTAKE_AIR 2
#define FIELD_RPM 3
#define FIELD_SPEED 4
#define FIELD_BATTERY 5
#define FIELD_COOLANT_TEMP 6
#define FIELD_INJECTION_TIME 7
