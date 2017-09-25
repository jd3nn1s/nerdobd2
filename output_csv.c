#include "plugins.h"
#include "common.h"

#include <time.h>
#include <string.h>
#include <unistd.h>

// Use fields recommended by RaceRender at http://racerender.com/Developer/DataFormat.html

static FILE* csv_fh;

void output_double(double val, bool delimiter) {
    fprintf(csv_fh, "%f", val);
    if (delimiter)
        fputc(',', csv_fh);
}

void output_csv_handle_data(obd_data_t *obd) {
    struct timeval tv;
    double time_val;

    gettimeofday(&tv, NULL);
    time_val = tv.tv_sec + ((double)tv.tv_usec / 1000000);
    output_double(time_val, true);
    output_double(obd->longitude, true);
    output_double(obd->latitude, true);
    output_double(obd->altitude, true);
    fprintf(csv_fh, ",,");    // Skip g-force values
    output_double(obd->speed, true);
    output_double(obd->track, true);
    fputc(',', csv_fh);    // Skip lap
    output_double(obd->rpm, true);
    fprintf(csv_fh, ",,,");    // Skip gear, throttle and gps mode
    fprintf(csv_fh, "\r\n");
}

void output_csv_close() {
    if (csv_fh)
        fclose(csv_fh);
}

static struct output_plugin output_csv_plugin = {
    .handle_data = output_csv_handle_data,
    .close = output_csv_close
};

struct output_plugin* output_plugin_load(void)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char buf[128];
    bool print_header = true;

    snprintf(buf, sizeof(buf), "csv/%02d%02d%02d-%02d%02d00.csv", tm.tm_year + 1900,
                                                                tm.tm_mon + 1,
                                                                tm.tm_mday,
                                                                tm.tm_hour,
                                                                tm.tm_min);

    if (access(buf, F_OK) == 0)
        print_header = false;

    csv_fh = fopen(buf, "ab");
    if (csv_fh == NULL) {
        fprintf(stderr, "Unable to open %s: ", buf);
        perror(NULL);
        return NULL;
    }

    if (print_header)
        fprintf(csv_fh, "Time,Longitude,Latitude,Altitude,X,Y,MPH,Heading,Lap,RPM,Gear,Throttle,Accuracy\r\n"); 
    return &output_csv_plugin;
}
