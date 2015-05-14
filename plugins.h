#include <stdbool.h>
#include <dlfcn.h>
#include <stdio.h>

#include "common.h"

/*
int     exec_query(PGconn *, char *);
*/

struct output_plugin {
    void (*output_handle_data)(obd_data_t* obd);
    void (*output_set_point)(const char* name);
    void (*output_close)();
};

struct output_plugin* output_plugin_load(void);

bool load_output_plugins(void);
void output_plugins_handle_data(obd_data_t*);
typedef struct output_plugin*(*output_plugin_load_t)(void);
