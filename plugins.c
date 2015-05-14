#include "plugins.h"

#define MAX_PLUGINS 10

static void* loaded_plugins[MAX_PLUGINS] = {0};
static struct output_plugin* loaded_plugins_structs[MAX_PLUGINS] = {0};

bool load_output_plugins(void) {
    void *handle;
    const char* plugin_file = "./liboutput_postgres.so";
    char* error;
    
    handle = dlopen(plugin_file, RTLD_NOW);

    if (handle == NULL) {
        fprintf(stderr, "Unable to load %s: %s\n", plugin_file, dlerror());
        return false;
    }
    // clear any error conditions
    dlerror();
    output_plugin_load_t plugin_load = dlsym(handle, "output_plugin_load");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        dlclose(handle);
        return false;
    }
    printf("Loaded: %s\n", plugin_file);
    loaded_plugins_structs[0] = plugin_load();
    loaded_plugins[0] = handle;
    return true;
}

void init_output_plugins() {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        
    }        
}

void close_output_plugins() {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->output_close)
            loaded_plugins_structs[i]->output_close();
        loaded_plugins_structs[i] = 0;
        dlclose(loaded_plugins[i]);
        loaded_plugins[i] = 0;
    }
}

void output_plugins_handle_data(obd_data_t* obd) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->output_handle_data)
            loaded_plugins_structs[i]->output_handle_data(obd);
    }
}

void output_plugins_set_point(const char* name) {
    int i = 0;
    for (; loaded_plugins[i] && i < MAX_PLUGINS; i++) {
        if (loaded_plugins_structs[i]->output_set_point)
            loaded_plugins_structs[i]->output_set_point(name);
    }
}
