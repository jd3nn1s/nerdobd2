#include "core.h"

void    cleanup(int);
char    cleaning_up = 0;

#ifdef BUILD_HTTPD
pid_t   pid_httpd = -1;
#endif

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


int
main(int argc, char **argv) {
    int     ret;

    // catch orphans
    signal(SIGCHLD, sig_chld);

    if (!load_output_plugins()) {
        fprintf(stderr, "Unable to initialize output plugins.\n");
        return -1;
    }

#ifdef BUILD_HTTPD
    puts("firing up httpd server");
    if ((pid_httpd = httpd_start()) == -1)
        cleanup(15);
#endif

#ifdef GPSD_FOUND
    // collect gps data
    if (gps_start() == 0)
        puts("gpsd connection established, collecting gps data");
    else
        puts("gps data not available");
#endif

    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);


#ifdef TEST
    output_plugins_set_point("startup");
    // for testing purposes
    int     i = 0, flag = 0;
    for (;;) {
        handle_data("rpm", 1000 + i * 100, 0);
        handle_data("injection_time", 0.15 * i, 1);
        handle_data("oil_pressure", i, 0);
        handle_data("temp_engine", 90, 0);
        handle_data("temp_air_intake", 35, 0);
        handle_data("voltage", 0.01 * i, 0);

        if (!i % 15)
            handle_data("speed", 0, 1);
        else
            handle_data("speed", 3 * i, 1);

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
insert_data(obd_data_t obd) {

#ifdef GPSD_FOUND
   static struct gps_fix_t gps;
   if (get_gps_data(&gps) == 0)
       obd.gps = gps;
#endif

    output_plugins_handle_data(&obd);
}

// this struct collects all engine data
// before sending it to the output plugins
obd_data_t obd_data;

void
handle_data(char *name, float value, float duration) {
    /* first block gets rpm, injection time, oil pressure
     * second block gets speed
     * third block gehts voltage and temperatures (not as often requested)
     */

    if (!strcmp(name, "temp_engine"))
        obd_data.temp_engine = value;
    else if (!strcmp(name, "temp_air_intake"))
        obd_data.temp_air_intake = value;
    else if (!strcmp(name, "voltage"))
        obd_data.voltage = value;

    else if (!strcmp(name, "rpm"))
        obd_data.rpm = value;
    else if (!strcmp(name, "injection_time")) {
        obd_data.injection_time = value;
        obd_data.duration_consumption = duration;
    }
    else if (!strcmp(name, "oil_pressure"))
        obd_data.oil_pressure = value;

    // everytime we get speed, calculate consumption
    // end send data to output plugins
    else if (!strcmp(name, "speed")) {
        obd_data.speed = value;

        // calculate consumption per hour
        obd_data.consumption_per_h =
            MULTIPLIER * obd_data.rpm * obd_data.injection_time;

        // calculate consumption per 100km
        if (obd_data.speed > 0)
            obd_data.consumption_per_100km =
                obd_data.consumption_per_h / obd_data.speed * 100;
        else
            obd_data.consumption_per_100km = NAN;


        obd_data.duration_speed = duration;

        insert_data(obd_data);
    }
}
