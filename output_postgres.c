#include "output_postgres.h"

PGconn *db;

void
close_db() {
    PQfinish(db);
    db = 0;
}

int
exec_query(char *query) {
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
            return exec_query(query);
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


bool
open_db(void) {

    db = PQconnectdb(DB_POSTGRES);
    if (PQstatus(db) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s",
                PQerrorMessage(db));
        close_db();
        return false;
    }
    return true;
}


void
init_db(void) {
    // create data table
    exec_query("CREATE TABLE IF NOT EXISTS data ( \
                        id                    SERIAL, \
                        time                  TIMESTAMP, \
                        rpm                   FLOAT, \
                        oil_pressure          FLOAT, \
                        speed                 FLOAT, \
                        fuel_remaining        FLOAT, \
                        fuel_level            INTEGER, \
                        temp_oil              FLOAT, \
                        temp_coolant          FLOAT, \
                        temp_air_intake       FLOAT, \
                        voltage               FLOAT, \
                        latitude              FLOAT, \
                        longitude             FLOAT, \
                        altitude              FLOAT, \
                        gps_track             FLOAT, \
                        gps_speed             FLOAT)");


    // create table where set point information is stored
    exec_query("CREATE TABLE IF NOT EXISTS setpoints ( \
                        id          SERIAL, \
                        name        VARCHAR, \
                        time        TIMESTAMP, \
                        data        INTEGER)");


    // since postgres doesn't support replace into, we're defining our own set_setpoints function
    exec_query("CREATE OR REPLACE FUNCTION set_setpoint(text) RETURNS void AS $$ \
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

    exec_query("BEGIN TRANSACTION");

    strncpy(query,
            "INSERT INTO data VALUES ( DEFAULT, current_timestamp",
            sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';

    build_query(query, sizeof(query), obd);
    exec_query(query);

    exec_query("END TRANSACTION");
}

void output_postgres_set_point(const char* name) {
    exec_query("SELECT set_setpoint('startup')");
}

void output_postgres_close() {
    printf("postgres closing\n");
    close_db();
}

static struct output_plugin output_postgres_plugin = {
    .handle_data = output_postgres_handle_data,
    .set_point = output_postgres_set_point,
    .close = output_postgres_close
};

struct output_plugin* output_plugin_load(void)
{
    printf("output_postgres doing load.\n");
    if (!open_db())
        return NULL;
    init_db();
    return &output_postgres_plugin;
}

static struct query_plugin query_postgres_plugin = {
    .json_get_data = json_get_data,
    .json_get_averages = json_get_averages,
    .json_get_graph_data = json_get_graph_data,
    .close = output_postgres_close
};

struct query_plugin* query_plugin_load(void) {
    if (!open_db())
        return NULL;
    return &query_postgres_plugin;
}
