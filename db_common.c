#include "common.h"
#include "db_common.h"

#include <string.h>
#include <stdio.h>

void
add_value(char *s, double value) {
    char    buffer[LEN_QUERY];

    strncpy(buffer, s, LEN_QUERY);

    if (!isnan(value))
        snprintf(s, LEN_QUERY, "%s, %f", buffer, value);
    else
        snprintf(s, LEN_QUERY, "%s, 'NaN'", buffer);
}

void
build_query(char* query, int query_len, obd_data_t* obd) {
    // add obd data
    add_value(query, obd->rpm);
    add_value(query, obd->speed);
    add_value(query, obd->injection_time);
    add_value(query, obd->oil_pressure);
    add_value(query, obd->consumption_per_100km);
    add_value(query, obd->consumption_per_h);
    add_value(query, obd->duration_consumption);
    add_value(query, obd->duration_speed);
    add_value(query, obd->consumption_per_h / 3600 * obd->duration_consumption);
    add_value(query, obd->speed / 3600 * obd->duration_speed);
    add_value(query, obd->temp_engine);
    add_value(query, obd->temp_air_intake);
    add_value(query, obd->voltage);

#ifdef GPSD_FOUND
    // add gps data, if available
    if (obd->gps.mode != 0 && obd->gps.mode != 1) {
        add_value(query, (double) obd->gps.mode);
        add_value(query, obd->gps.latitude);
        add_value(query, obd->gps.longitude);
        add_value(query, obd->gps.altitude);
        add_value(query, obd->gps.speed);
        add_value(query, obd->gps.climb);
        add_value(query, obd->gps.track);
        add_value(query, obd->gps.epy);
        add_value(query, obd->gps.epx);
        add_value(query, obd->gps.epv);
        add_value(query, obd->gps.eps);
        add_value(query, obd->gps.epc);
        add_value(query, obd->gps.epd);
    }

#else
    if (0);
#endif
    else {
        // fill in empty column fields for gps data
        strncat(query,
                ", 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN', 'NaN'",
                query_len - strlen(query) - 1);
        query[query_len - 1] = '\0';
    }

    strncat(query, " )\n", query_len - strlen(query) - 1);
    query[query_len - 1] = '\0';
}
