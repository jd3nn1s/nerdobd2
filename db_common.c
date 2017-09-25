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
    add_value(query, obd->oil_pressure);
    add_value(query, obd->speed);
    add_value(query, obd->fuel_remaining);
    add_value(query, obd->fuel_level);
    add_value(query, obd->temp_oil);
    add_value(query, obd->temp_coolant);
    add_value(query, obd->temp_air_intake);
    add_value(query, obd->voltage);

#ifdef GPSD_FOUND
    // add gps data
    add_value(query, obd->latitude);
    add_value(query, obd->longitude);
    add_value(query, obd->altitude);
    add_value(query, obd->track);
    add_value(query, obd->gps_speed);
#else
    // fill in empty column fields for gps data
    strncat(query,
            ", 'NaN', 'NaN', 'NaN', 'NaN', 'NaN'",
            query_len - strlen(query) - 1);
    query[query_len - 1] = '\0';
#endif
    add_value(query, obd->throttle_angle);

    strncat(query, " )\n", query_len - strlen(query) - 1);
    query[query_len - 1] = '\0';
}
