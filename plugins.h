#include <stdbool.h>
#include <dlfcn.h>
#include <stdio.h>
#include <json/json.h>

#include "common.h"

struct output_plugin {
    void (*handle_data)(obd_data_t* obd);
    void (*set_point)(const char* name);
    void (*close)(void);
};

bool load_output_plugins(void);
void close_output_plugins(void);
struct output_plugin* output_plugin_load(void);
void output_plugins_handle_data(obd_data_t*);
typedef struct output_plugin*(*output_plugin_load_t)(void);
bool load_plugin(const char*, bool);

// query plugin
struct query_plugin {
    json_object* (*json_get_data)(void);
    json_object* (*json_get_averages)(void);
    json_object* (*json_get_graph_data)(const char*, unsigned long int, unsigned long int);
    void (*close)(void);
};

json_object* query_plugin_json_get_data(void);
json_object* query_plugin_json_get_averages(void);
json_object* query_plugin_json_get_graph_data(const char*, unsigned long int, unsigned long int);

bool load_query_plugin(void);
void close_query_plugin(void);
typedef struct query_plugin*(*query_plugin_load_t)(void);

