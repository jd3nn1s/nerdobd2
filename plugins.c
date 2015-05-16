#include <ini.h>
#include "plugins.h"
#include "config.h"

#define MAX_PLUGINS 10

static void* loaded_plugins[MAX_PLUGINS] = {NULL};
static struct output_plugin* loaded_plugins_structs[MAX_PLUGINS] = {NULL};
static struct query_plugin* loaded_query_plugin_struct = NULL;

static int output_ini_handler(void* user, const char* section, const char* name, const char* value) {
    char file_name[1024];
    if (strcmp(section, "plugins"))
        return 1;

    if (strcmp(name, "output"))
        return 1;

    snprintf(file_name, sizeof(file_name), "./lib%s.so", value);
    if (!load_plugin(file_name, false)) 
        return 0;
    return 1;
}

bool load_output_plugins(void) {
    bool error;
    printf("Loading output plugins...\n");
    error = (ini_parse(INI_CONFIG, output_ini_handler, NULL) > 0);
    if (error)
        fprintf(stderr, "Unable to load output plugins!\n");
    return !error;
}

static int query_ini_handler(void* user, const char* section, const char* name, const char* value) {
    char file_name[1024];
    static bool loaded = false;
    if (strcmp(section, "plugins"))
        return 1;

    if (strcmp(name, "query"))
        return 1;

    if (loaded) {
        fprintf(stderr, "You can only specify one query plugin.\n");
        return 0;
    }

    snprintf(file_name, sizeof(file_name), "./lib%s.so", value);
    if (!load_plugin(file_name, true))
        return 0;
    return 1;
}


bool load_query_plugin(void)
{
    bool error;
    printf("Loading query plugin...\n");
    error = (ini_parse(INI_CONFIG, query_ini_handler, NULL) > 0);
    if (error || loaded_query_plugin_struct == NULL) {
        fprintf(stderr, "Unable to load query plugin!\n");
        return false;
    }
    return true;
}

bool load_plugin(const char* plugin_file, bool query_plugin) {
    void *handle;
    char* error;
    char* sym_name;
    output_plugin_load_t output_plugin_load;
    query_plugin_load_t query_plugin_load;
    
    handle = dlopen(plugin_file, RTLD_NOW);

    if (handle == NULL) {
        fprintf(stderr, "Unable to load %s: %s\n", plugin_file, dlerror());
        return false;
    }
    // clear any error conditions
    dlerror();

    if (query_plugin)
        query_plugin_load = dlsym(handle, "query_plugin_load");
    else
        output_plugin_load = dlsym(handle, "output_plugin_load");

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        dlclose(handle);
        return false;
    }
    printf("Loaded: %s\n", plugin_file);

    if (query_plugin) {
        loaded_query_plugin_struct = query_plugin_load();
        if (!loaded_query_plugin_struct->json_get_data
            || !loaded_query_plugin_struct->json_get_averages
            || !loaded_query_plugin_struct->json_get_graph_data)
              fprintf(stderr, "Configured query plugin does not implement all required capabilities.\n");
    }
    else {
        loaded_plugins_structs[0] = output_plugin_load();
        loaded_plugins[0] = handle;
    }
    return true;
}

void init_output_plugins(void) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        
    }        
}

// Output Plugin Interface

void close_output_plugins(void) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->close)
            loaded_plugins_structs[i]->close();
        loaded_plugins_structs[i] = 0;
        dlclose(loaded_plugins[i]);
        loaded_plugins[i] = 0;
    }
}

void output_plugins_handle_data(obd_data_t* obd) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->handle_data)
            loaded_plugins_structs[i]->handle_data(obd);
    }
}

void output_plugins_set_point(const char* name) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->set_point)
            loaded_plugins_structs[i]->set_point(name);
    }
}

// Query Plugin Interface

json_object* query_plugin_json_get_data() {
    return loaded_query_plugin_struct->json_get_data();
}

json_object* query_plugin_json_get_averages() {
    return loaded_query_plugin_struct->json_get_averages();
}

json_object* query_plugin_json_get_graph_data(const char * key, unsigned long int index, unsigned long int timespan) {
    return loaded_query_plugin_struct->json_get_graph_data(key, index, timespan);
}

void close_query_plugin(void) {
    if (loaded_query_plugin_struct->close)
        loaded_query_plugin_struct->close();
    loaded_query_plugin_struct = 0;
    dlclose(loaded_plugins[0]);
    loaded_plugins[0] = 0;
}

