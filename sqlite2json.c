#include "output_sqlite.h"
#include "json.h"
#include <math.h>

extern sqlite3 *db;

// calls add_double, but checks if value actually is a number.
void
_add_double(json_object * parent, char *key, sqlite3_stmt * stmt, int column) {
    if (sqlite3_column_type(stmt, column) == SQLITE_FLOAT ||
        sqlite3_column_type(stmt, column) == SQLITE_INTEGER)
        add_double(parent, key, sqlite3_column_double(stmt, column));
    else
        add_string(parent, key, "null");        // json doesn't support NaN
}

json_object*
json_get_data(void) {
    json_object *data = json_object_new_object();

    char    query[LEN_QUERY];
    sqlite3_stmt *stmt;

    exec_query("BEGIN TRANSACTION");

    // query data
    snprintf(query, sizeof(query), "SELECT rpm, speed, injection_time, \
                     oil_pressure, consumption_per_100km, consumption_per_h, \
                     temp_engine, temp_air_intake, voltage, \
                     gps_mode, \
                     gps_latitude, gps_longitude, gps_altitude, \
                     gps_speed, gps_climb, gps_track, \
                     gps_err_latitude, gps_err_longitude, gps_err_altitude, \
                     gps_err_speed, gps_err_climb, gps_err_track \
              FROM data \
              ORDER BY id \
              DESC LIMIT 1");

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        _add_double(data, "rpm", stmt, 0);
        _add_double(data, "speed", stmt, 1);
        _add_double(data, "injection_time", stmt, 2);
        _add_double(data, "oil_pressure", stmt, 3);
        _add_double(data, "consumption_per_100km", stmt, 4);
        _add_double(data, "consumption_per_h", stmt, 5);
        _add_double(data, "temp_engine", stmt, 6);
        _add_double(data, "temp_air_intake", stmt, 7);
        _add_double(data, "voltage", stmt, 8);
        _add_double(data, "gps_mode", stmt, 9);
        _add_double(data, "gps_latitude", stmt, 10);
        _add_double(data, "gps_longitude", stmt, 11);
        _add_double(data, "gps_altitude", stmt, 12);
        _add_double(data, "gps_speed", stmt, 13);
        _add_double(data, "gps_climb", stmt, 14);
        _add_double(data, "gps_track", stmt, 15);
        _add_double(data, "gps_err_latitude", stmt, 16);
        _add_double(data, "gps_err_longitude", stmt, 17);
        _add_double(data, "gps_err_altitude", stmt, 18);
        _add_double(data, "gps_err_speed", stmt, 19);
        _add_double(data, "gps_err_climb", stmt, 20);
        _add_double(data, "gps_err_track", stmt, 21);
    }
    if (sqlite3_finalize(stmt) != SQLITE_OK) {
#ifdef DEBUG_DB
        printf("sqlite3_finalize() error\n");
#endif
        return NULL;
    }

    exec_query("END TRANSACTION");
    return data;
}


json_object*
json_get_averages(void) {
    json_object *data = json_object_new_object();

    char    query[LEN_QUERY];
    sqlite3_stmt *stmt;

    exec_query("BEGIN TRANSACTION");

    // average since last startup
    snprintf(query, sizeof(query),
             "SELECT strftime('%%s000', setpoints.time), \
                     SUM(data.speed * data.consumption_per_100km) / SUM(data.speed), \
                     SUM(data.liters), \
                     SUM(data.kilometers) \
              FROM   setpoints, data \
              WHERE  consumption_per_100km != -1 \
              AND    id > ( SELECT data FROM setpoints WHERE name = 'startup' )");

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        _add_double(data, "timestamp_startup", stmt, 0);
        _add_double(data, "consumption_average_startup", stmt, 1);
        _add_double(data, "consumption_liters_startup", stmt, 2);
        _add_double(data, "kilometers_startup", stmt, 3);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
#ifdef DEBUG_DB
        printf("sqlite3_finalize() error\n");
#endif
        return NULL;
    }

    // average since timespan
    /*
     * snprintf(query, sizeof(query),
     * "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
     * FROM data \
     * WHERE time > DATETIME('NOW', '-%lu seconds', 'localtime') \
     * AND consumption_per_100km != -1",
     * timespan);
     */


    // overall consumption average
    snprintf(query, sizeof(query),
             "SELECT SUM(speed * consumption_per_100km) / SUM(speed), \
                     SUM(liters), \
                     SUM(kilometers) \
              FROM   data \
              WHERE  consumption_per_100km != -1");

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        _add_double(data, "consumption_average_total", stmt, 0);
        _add_double(data, "consumption_liters_total", stmt, 1);
        _add_double(data, "kilometers_total", stmt, 2);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
#ifdef DEBUG_DB
        printf("sqlite3_finalize() error\n");
#endif
        return NULL;
    }

    exec_query("END TRANSACTION");
    return data;
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
json_object*
json_get_graph_data(const char *key, unsigned long int index,
                unsigned long int timespan) {
    char    query[LEN_QUERY];
    sqlite3_stmt *stmt;

    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    exec_query("BEGIN TRANSACTION");

    snprintf(query, sizeof(query), "SELECT id, strftime('%%s000', time), %s \
              FROM   data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds', 'localtime') \
              ORDER BY id", key, index, timespan);

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) !=
        SQLITE_OK) {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        add_data(data, sqlite3_column_double(stmt, 1),
                 sqlite3_column_double(stmt, 2));

        index = sqlite3_column_int(stmt, 0);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
#ifdef DEBUG_DB
        printf("sqlite3_finalize() error\n");
#endif
        return NULL;
    }

    exec_query("END TRANSACTION");

    add_int(graph, "index", index);

    return graph;
}
