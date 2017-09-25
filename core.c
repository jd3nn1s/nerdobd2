#include "core.h"
#include <pthread.h>

void cleanup(int);
char cleaning_up = 0;

static pthread_t output_thread;
static const int output_delay_usec = 50 * 1000;

#ifdef BUILD_HTTPD
pid_t   pid_httpd = -1;
#endif

// this struct collects all engine data
// before sending it to the output plugins
obd_data_t obd_data;
static pthread_mutex_t obd_data_lock = PTHREAD_MUTEX_INITIALIZER;

void
wait4childs(void) {
    do {
        wait(NULL);
    } while (errno != ECHILD);

    return;
}

void
cleanup(int signo) {
    // if we're already cleaning up, do nothing
    if (cleaning_up)
        return;

    cleaning_up = 1;

    if (output_thread) {
        puts("stopping output thread...");
        pthread_cancel(output_thread);
        pthread_join(output_thread, NULL);
        puts("output thread stopped.");
    }

#ifdef BUILD_HTTPD
    // shutdown httpd
    printf("sending SIGTERM to httpd (%d)\n", pid_httpd);
    if (pid_httpd != -1)
        kill(pid_httpd, SIGTERM);
#endif

    close_output_plugins();

    // close serial port
    printf("shutting down serial port...\n");
    kw1281_close();

    gps_stop();

    // wait for all child processes
    printf("waiting for child processes to finish...\n");
    wait4childs();

    printf("exiting\n");
    exit(0);
}


void
sig_chld(int signo) {
    pid_t   pid;
    int     stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

static void* output_telemetry(void* arg) {
    while (1) {
        obd_data_t tmp_data = obd_data;

        pthread_mutex_lock(&obd_data_lock);
        tmp_data = obd_data;
        pthread_mutex_unlock(&obd_data_lock);

        output_plugins_handle_data(&tmp_data);

        usleep(output_delay_usec);
    }
}

int
main(int argc, char **argv) {
    int     ret;

    // catch orphans
    signal(SIGCHLD, sig_chld);

#ifdef BUILD_HTTPD
    // httpd should be started before we load plugins
    // so that the httpd process does not have any output plugins loaded
    puts("firing up httpd server");
    if ((pid_httpd = httpd_start()) == -1)
        cleanup(15);
#endif

    if (!load_output_plugins()) {
        fprintf(stderr, "Unable to initialize output plugins.\n");

#ifdef BUILD_HTTPD
    if (pid_httpd != -1)
         kill(pid_httpd, SIGTERM);
#endif
        return -1;
    }

    // collect gps data
    if (gps_start(&obd_data, &obd_data_lock))
        puts("gps thread started");
    else
        perror("gps data not available");

    // collect arduino data
    if (secondary_telemetry_start(&obd_data, &obd_data_lock))
        puts("secondary telemetry thread started");
    else
        perror("secondary telemetry thread could not start");

    if (pthread_create(&output_thread, NULL, output_telemetry, 0)) {
        perror("unable to create output thread");
        cleanup(15);
    }

    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

#ifdef TEST
    output_plugins_set_point("startup");
    // for testing purposes
    int     i = 0, flag = 0;
    for (;;) {
        block_handle_field(FIELD_RPM, 1000 + i * 100);
        //block_handle_field(FIELD_COOLANT_TEMP, 90);
        block_handle_field(FIELD_INTAKE_AIR, 35);
        block_handle_field(FIELD_BATTERY, 0.1 * i);
        block_handle_field(FIELD_THROTTLE_ANGLE, i);

        if (!i % 15)
            block_handle_field(FIELD_SPEED, 0);
        else
            block_handle_field(FIELD_SPEED, 3 * i);

        usleep(300000);


        if (i > 35)
            flag = 1;
        if (i < 2)
            flag = 0;

        if (flag)
            i--;
        else
            i++;
    }
#endif

    ret = SERIAL_HARD_ERROR;

    for (;;) {
        if (ret == SERIAL_HARD_ERROR) {
            while (kw1281_open(DEVICE) == -1)
                usleep(500000);

            /* since this restarts the serial connection,
             * it is probably due to a new ride was started, since we
             * set the startup set point to the last index we can find
             */
            output_plugins_set_point("startup");

            // set the first run flags
            consumption_first_run = 1;
            speed_first_run = 1;
        }

        printf("init\n");

        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init(0x01, ret);

        // soft error, e.g. communication error
        if (ret == SERIAL_SOFT_ERROR) {
            printf("init failed, retrying...\n");
            continue;
        }

        // hard error (e.g. serial cable unplugged)
        else if (ret == SERIAL_HARD_ERROR) {
            printf("serial port error, trying to recover...\n");
            kw1281_close();
            continue;
        }


        // start main loop, restart on errors
        if (kw1281_mainloop() == SERIAL_SOFT_ERROR) {
            printf("errors. restarting...\n");
            continue;
        }
    }

    // should never be reached
    return 0;
}

void
block_handle_field(int type, float value) {

    pthread_mutex_lock(&obd_data_lock);
    switch (type) {
    case FIELD_COOLANT_TEMP:
        obd_data.temp_coolant = value;
        break;
    case FIELD_INTAKE_AIR:
        obd_data.temp_air_intake = value;
        break;
    case FIELD_BATTERY:
        obd_data.voltage = value;
        break;
    case FIELD_RPM:
        obd_data.rpm = value;
        break;
    case FIELD_SPEED:
        obd_data.speed = value;
        break;
    case FIELD_THROTTLE_ANGLE:
        obd_data.throttle_angle = value;
        break;
    }
    pthread_mutex_unlock(&obd_data_lock);
}
