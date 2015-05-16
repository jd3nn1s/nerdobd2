#include "httpd.h"
#include "plugins.h"
#include <mongoose.h>
#include <signal.h>

struct mg_server *server;

void
httpd_stop(int signo) {
    printf(" - child (httpd): closing connections...\n");
    mg_destroy_server(&server);
    close_query_plugin();
    do {
        wait(NULL);
    } while (errno != ECHILD);

    _exit(0);
}

long get_query_long(struct mg_connection *conn, const char* name)
{
    char buf[128];
    long int lvalue = 0;
    if (mg_get_var(conn, name, buf, sizeof(buf)) <= 0) {
        return 0;
    }
    return strtol(buf, NULL, 10);
}

int handle_request(struct mg_connection *conn) {
    if (!strcmp(conn->uri, "/data.json")) {
        mg_send_header(conn, "Content-Type", "application/json");
        json_object* json = query_plugin_json_get_data();
        mg_printf_data(conn, "%s", json_object_to_json_string(json));
        json_object_put(json);
    }
    else if (!strcmp(conn->uri, "/averages.json")) {
        mg_send_header(conn, "Content-Type", "application/json");
        json_object* json = query_plugin_json_get_averages();
        mg_printf_data(conn, "%s", json_object_to_json_string(json));
        json_object_put(json);
    }
    else if (!strcmp(conn->uri, "/consumption.json")) {
        mg_send_header(conn, "Content-Type", "application/json");
        json_object* json = query_plugin_json_get_graph_data("consumption_per_100km", get_query_long(conn, "index"), get_query_long(conn, "timespan"));
        mg_printf_data(conn, "%s", json_object_to_json_string(json));
        json_object_put(json);
    }
    else if (!strcmp(conn->uri, "/speed.json")) {
        mg_send_header(conn, "Content-Type", "application/json");
        json_object* json = query_plugin_json_get_graph_data("speed", get_query_long(conn, "index"), get_query_long(conn, "timespan"));
        mg_printf_data(conn, "%s", json_object_to_json_string(json));
        json_object_put(json);
    }
    else if (!strcmp(conn->uri, "/gps_altitude.json")) {
        mg_send_header(conn, "Content-Type", "application/json");
        json_object* json = query_plugin_json_get_graph_data("gps_altitude", get_query_long(conn, "index"), get_query_long(conn, "timespan"));
        mg_printf_data(conn, "%s", json_object_to_json_string(json));
        json_object_put(json);
    }
    else
        return MG_FALSE;

    return MG_TRUE;
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
    switch (ev) {
    case MG_AUTH:
        return MG_TRUE;
    case MG_REQUEST:
        return handle_request(conn);
    default:
        return MG_FALSE;
    }
}

int
httpd_start(void) {
    int     s;
    pid_t   pid;

    if ((pid = fork()) == 0) {

        if (!load_query_plugin()) {
            fprintf(stderr, "Unable to load query plugin, httpd cannot start.\n");
        }

        server = mg_create_server(NULL, ev_handler);
        mg_set_option(server, "listening_port", HTTPD_PORT);
        mg_set_option(server, "document_root", DOCROOT);

        // add signal handler for cleanup function
        signal(SIGINT, httpd_stop);
        signal(SIGTERM, httpd_stop);

        for (;;) {
            mg_poll_server(server, 1000);
        }

        // should never be reached
        _exit(0);
    }

    return pid;
}
