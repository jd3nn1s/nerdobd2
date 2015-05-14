#include "output_postgres.h"

static PGconn *db;

void
close_db(PGconn * db) {
    PQfinish(db);
}

int
exec_query(PGconn * db, char *query) {
    PGresult *res;

#ifdef DEBUG_DB
    if (strstr(query, "TRANSACTION") == NULL)
        printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    switch (PQresultStatus(res)) {

        case PGRES_TUPLES_OK:
        case PGRES_COMMAND_OK:
            // all OK, no data to process
            PQclear(res);
            return 0;
            break;

        case PGRES_EMPTY_QUERY:
            // server had nothing to do, a bug maybe?
            fprintf(stderr, "query '%s' failed (EMPTY_QUERY): %s\n", query,
                    PQerrorMessage(db));
            PQclear(res);
            return -1;
            break;

        case PGRES_NONFATAL_ERROR:
            // can continue, possibly retry the command
            fprintf(stderr, "query '%s' failed (NONFATAL, retrying): %s\n",
                    query, PQerrorMessage(db));
            PQclear(res);
            return exec_query(db, query);
            break;

        case PGRES_BAD_RESPONSE:
        case PGRES_FATAL_ERROR:
        default:
            // fatal or unknown error, cannot continue
            fprintf(stderr, "query '%s' failed: %s\n", query,
                    PQerrorMessage(db));
    }

    return -1;
}


PGconn *
open_db(void) {
    PGconn *db;

    db = PQconnectdb(DB_POSTGRES);
    if (PQstatus(db) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s",
                PQerrorMessage(db));
        close_db(db);
        return NULL;
    }

    return db;
}


void
init_db(PGconn * db) {
    // create data table
    exec_query(db, "CREATE TABLE IF NOT EXISTS data ( \
                        id                    SERIAL, \
                        time                  TIMESTAMP, \
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
                        gps_mode              FLOAT, \
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
    exec_query(db, "CREATE TABLE IF NOT EXISTS setpoints ( \
                        id          SERIAL, \
                        name        VARCHAR, \
                        time        TIMESTAMP, \
                        data        INTEGER)");


    // since postgres doesn't support replace into, we're defining our own set_setpoints function
    exec_query(db,
               "CREATE OR REPLACE FUNCTION set_setpoint(text) RETURNS void AS $$ \
               BEGIN \
                   IF EXISTS( SELECT name FROM setpoints WHERE name = $1 ) THEN \
                       UPDATE setpoints SET time = current_timestamp, data = ( \
                           SELECT CASE WHEN count(*) = 0 \
                           THEN 0 \
                           ELSE id END \
                           FROM data \
                           GROUP BY id \
                           ORDER BY id DESC LIMIT 1 ) \
                       WHERE name = 'startup'; \
                   ELSE \
                       INSERT INTO setpoints VALUES( DEFAULT, $1, current_timestamp, ( \
                           SELECT CASE WHEN count(*) = 0 \
                           THEN 0 \
                           ELSE id END \
                           FROM data \
                           GROUP BY data.id \
                           ORDER BY data.id DESC LIMIT 1 ) ); \
                   END IF; \
                   RETURN; \
               END; \
               $$ LANGUAGE plpgsql;");

    return;
}



void output_postgres_handle_data(obd_data_t *obd) {
    char query[LEN_QUERY];

    exec_query(db, "BEGIN TRANSACTION");

    strncpy(query,
            "INSERT INTO data VALUES ( DEFAULT, current_timestamp",
            sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';

    build_query(query, sizeof(query), obd);
    exec_query(db, query);

    exec_query(db, "END TRANSACTION");
}

void output_postgres_set_point(const char* name) {
    exec_query(db, "SELECT set_setpoint('startup')");
}

void output_postgres_close(void* data) {
    printf("postgres closing\n");
    close_db(db);
}

static struct output_plugin output_postgres_plugin = {
    .output_handle_data = output_postgres_handle_data,
    .output_set_point = output_postgres_set_point,
    .output_close = output_postgres_close
};

struct output_plugin* output_plugin_load(void)
{
    printf("output_postgres doing load.\n");
    if ((db = open_db()) == NULL)
        return NULL;
    init_db(db);
    return &output_postgres_plugin;
}

