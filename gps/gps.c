//#include "gps.h"
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include "../common.h"
#include "datalogger.h"
#include "lowlevel.h"

static obd_data_t* obd_data;
static pthread_mutex_t* obd_data_lock;
static pthread_t thread;
static int fd = -1;

void gps_cleanup(void* arg) {
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

int32_t convert_bytes_to_int32(uint8_t* p) {
    return (int32_t)((*p << 24) | (*(++p) << 16) | (*(++p) << 8) | *(++p));
}

uint32_t convert_bytes_to_uint32(uint8_t* p) {
    return (uint32_t)convert_bytes_to_int32(p);
}

static void* thread_run(void* arg) {
    char* device = "/dev/ttyAMA0";

    while(fd == -1) {
        fd = open_port(device);
        if (fd == -1) {
            fprintf(stderr, "Failed to open gps device %s\n", device);
            sleep(1);
        }
    }
    printf("gps: port opened.\n");

    pthread_cleanup_push(gps_cleanup, (void*)0);

// configure GPS device
//    skytraq_output_enable_binary(fd, 1);
//    skytraq_set_power_mode(fd, 0, 1);
//    skytraq_set_position_rate(fd, 50, 1);
//    skytraq_set_serial_speed(fd, 6, 1);

    skytraq_read_software_version(fd);

    struct timespec clock = {0};
    clock_gettime(CLOCK_MONOTONIC, &clock);
    unsigned int clock_num = 0;

    while (1) {
        SkyTraqPackage* p = skytraq_read_next_package(fd, 1500);

        if (p) {
            if (p->data[0] == SKYTRAQ_RESPONSE_NAVIGATION_DATA) {

                if(p->data[1] == 0) { // we don't have a fix
                    if (++clock_num % 100 == 0) {
                        printf("no fix!\n");
                    }
                    continue;
                }

                uint16_t hdop = (p->data[29] << 8) | p->data[30];
                if (hdop >= 500) { // really poor horizontal dilution of precision
                    printf("poor resolution\n");
                    continue;
                }

                double latitude = convert_bytes_to_int32(&(p->data[9])) / pow(10, 7);
                double longitude = convert_bytes_to_int32(&(p->data[13])) / pow(10, 7);
                double altitude = convert_bytes_to_uint32(&(p->data[21])) / 100.0;
                double vx = convert_bytes_to_int32(&(p->data[47]));
                double vy = convert_bytes_to_int32(&(p->data[51]));
                double vz = convert_bytes_to_int32(&(p->data[55]));
                float speed = sqrt(pow(vx, 2) + pow(vy, 2));
                float track = atan(vx / vy);
                if (isnan(track))
                    track = 0;
                if (++clock_num % 100 == 0) {
                    struct timespec new_time = {0};
                    clock_gettime(CLOCK_MONOTONIC, &new_time);
                    printf("%fHz\n", 100 / (((double)(new_time.tv_sec - clock.tv_sec)) + (new_time.tv_nsec - clock.tv_nsec) / 1000000000));
                    clock = new_time;

                    printf("Latitude: %f Longitude: %f altitude: %fm speed: %f m/s track: %f deg hdop: %d\n", latitude, longitude, altitude, speed, track, hdop);
                }   
                pthread_mutex_lock(obd_data_lock);
                obd_data->latitude = latitude;
                obd_data->longitude = longitude;
                obd_data->altitude = altitude;
                obd_data->gps_speed = speed;
                obd_data->track = track;
                pthread_mutex_unlock(obd_data_lock);
            }
            else if (p->data[0] == SKYTRAQ_RESPONSE_POWER_MODE) {
                //printf("power: %d\n", p->data[1]);
            }
            else if (p->data[0] == SKYTRAQ_RESPONSE_POSITION_RATE) {
                //printf("position: %d\n", p->data[1]);
            }
            skytraq_free_package(p);
        }
    }
    pthread_cleanup_pop(1);
}

bool gps_start(obd_data_t* data, pthread_mutex_t* lock) {
    obd_data = data;
    obd_data_lock = lock;

    return (pthread_create(&thread, NULL, thread_run, 0) == 0);
}

bool gps_stop(void) {
    if (thread) {
        printf("gps: shutting down\n");
        if (pthread_cancel(thread) != 0)
            fprintf(stderr, "gps: Cannot cancel thread.\n");
        pthread_join(thread, NULL);
        printf("gps: done\n");
        thread = 0;
    }
    printf("gps: stop complete.\n");
}
