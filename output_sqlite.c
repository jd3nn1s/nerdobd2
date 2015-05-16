#include "output_sqlite.h"
#include "db_common.h"

sqlite3 *db;

static int
busy(void *unused __attribute__ ((unused)), int count) {
    usleep(500000);

#ifdef DEBUG_DB
    printf("retrying query...\n");
#endif

    // give up after 30 seconds
    return (count < 60);
}


int
exec_query(char *query) {
    char   *error = NULL;

#ifdef DEBUG_DB
    if (strstr(query, "TRANSACTION") == NULL)
        printf("sql: %s\n", query);
#endif

    if (sqlite3_exec(db, query, NULL, NULL, &error) != SQLITE_OK) {
        if (error != NULL) {
            fprintf(stderr, "sql error: %s\n", error);
            sqlite3_free(error);
        }
        else
            fprintf(stderr, "sql error: unknown\n");
        return -1;
    }

    return 0;
}


bool
open_db(void) {

    // open database file
    if (sqlite3_open(DB_SQLITE, &db) != SQLITE_OK) {
        printf("Couldn't open database: %s", DB_SQLITE);
        db = 0;
        return false;
    }

    // retry on busy errors
    sqlite3_busy_handler(db, busy, NULL);

    // disable waiting for write to be completed
    exec_query("PRAGMA synchronous = OFF");

    // disable journal
    exec_query("PRAGMA journal_mode = OFF");
    return true;
}


void
init_db() {
    exec_query("BEGIN TRANSACTION");

    // create data table
    exec_query("CREATE TABLE IF NOT EXISTS data ( \
                        id                    INTEGER PRIMARY KEY, \
                        time                  DATE, \
                        rpm                   FLOAT, \
                        speed                 FLOAT, \
                        injection_time        FLOAT, \
                        oil_pressure          FLOAT, \
                        consumption_per_100km FLOAT, \
                        consumption_per_h     FLOAT, \
                        duration_consumption  FLOAT, \
                        duration_speed        FLOAT, \
                        liters                FLOAT, \
                        kilometers            FLOAT, \
                        temp_engine           FLOAT, \
                        temp_air_intake       FLOAT, \
                        voltage               FLOAT, \
                        gps_mode              INTEGER, \
                        gps_latitude          FLOAT, \
                        gps_longitude         FLOAT, \
                        gps_altitude          FLOAT, \
                        gps_speed             FLOAT, \
                        gps_climb             FLOAT, \
                        gps_track             FLOAT, \
                        gps_err_latitude      FLOAT, \
                        gps_err_longitude     FLOAT, \
                        gps_err_altitude      FLOAT, \
                        gps_err_speed         FLOAT, \
                        gps_err_climb         FLOAT, \
                        gps_err_track         FLOAT )");


    // create table where set point information is stored
    exec_query("CREATE TABLE IF NOT EXISTS setpoints ( \
                        name        VARCHAR PRIMARY KEY, \
                        time        DATE, \
                        data        INTEGER)");

    exec_query("END TRANSACTION");

    return;
}

void
close_db() {
    sqlite3_close(db);
}

void output_sqlite_handle_data(obd_data_t *obd) {
    char query[LEN_QUERY];

    exec_query("BEGIN TRANSACTION");

    strncpy(query,
            "INSERT INTO data VALUES ( NULL, DATETIME('now', 'localtime')",
            sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';

    build_query(query, sizeof(query), obd);
    exec_query(query);

    exec_query("END TRANSACTION");
}

void output_sqlite_set_point(const char* name) {
    exec_query("INSERT OR REPLACE INTO setpoints VALUES ( \
                   'startup', DATETIME('now', 'localtime'), ( \
                   SELECT CASE WHEN count(*) = 0 \
                   THEN 0 \
                   ELSE id END \
                   FROM data \
                   ORDER BY id DESC LIMIT 1 \
                   ) \
               )");
}

void output_sqlite_close(void) {
    printf("sqlite closing.\n");
    close_db();
}

static struct output_plugin output_sqlite_plugin = {
    .handle_data = output_sqlite_handle_data,
    .set_point = output_sqlite_set_point,
    .close = output_sqlite_close,
};

static struct query_plugin query_sqlite_plugin = {
    .json_get_data = json_get_data,
    .json_get_averages = json_get_averages,
    .json_get_graph_data = json_get_graph_data,
    .close = output_sqlite_close
};

struct output_plugin* output_plugin_load(void) {
    if (!open_db())
        return NULL;
    init_db(db);
    return &output_sqlite_plugin;
}

struct query_plugin* query_plugin_load(void) {
    if (!open_db())
        return NULL;
    return &query_sqlite_plugin;
}
