#include "common.h"

#include <pthread.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>

static const delay_usec = 100 * 1000;

static obd_data_t* obd_data;
static pthread_mutex_t* obd_data_lock;
static pthread_t thread;
static int fd = 0;

struct ArduinoTelemetry {
  float temp_coolant;
  float temp_oil;
  float fuel_remaining;
  uint8_t fuel_level;
} __attribute__((__packed__)) telemetry;

void arduino_cleanup(void* arg) {
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
}

static void* thread_run(void* arg) {

    pthread_cleanup_push(arduino_cleanup, (void*)0);

    unsigned int count = 0;

    while (1) {
        const unsigned int size = sizeof(telemetry);
        if (read(fd, (uint8_t*)&telemetry, size) != size) {
            perror("Unable to read data");
        }
        if (++count == 50) {
            printf("arduino: %f %f %f %d\n", telemetry.temp_coolant, telemetry.temp_oil, telemetry.fuel_remaining, telemetry.fuel_level);
            count = 0;
        }

        usleep(100000);        
        pthread_mutex_lock(obd_data_lock);
        obd_data->fuel_remaining = telemetry.fuel_remaining;
        obd_data->fuel_level = telemetry.fuel_level;
        obd_data->temp_oil = telemetry.temp_oil;
        obd_data->temp_coolant = telemetry.temp_coolant;
        pthread_mutex_unlock(obd_data_lock);

        usleep(delay_usec);
    }
    pthread_cleanup_pop(1);

}

bool secondary_telemetry_start(obd_data_t* data, pthread_mutex_t* lock) {
    obd_data = data;
    obd_data_lock = lock;

    fd = open("/dev/i2c-1", O_RDWR);
    if (fd == -1) {
        perror("Unable to open I2C bus");
        return false;
    }

    int addr = 0x4;
    if (ioctl(fd, I2C_SLAVE, addr) == -1) {
        perror("cannot set i2c slave address");
        return false;
    }

    return (pthread_create(&thread, NULL, thread_run, 0) == 0);
}

bool secondary_telemetry_stop(void) {
    if (thread) {
        printf("arduino: shutting down\n");
        if (pthread_cancel(thread) != 0)
            fprintf(stderr, "arduino: Cannot cancel thread.\n");
        printf("arduino: joining\n");
        pthread_join(thread, NULL);
        printf("arduino: done\n");
        thread = 0;
    }
    printf("arduino: stop complete.\n");
}
