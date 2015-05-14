#include "plugins.h"
#include "common.h"

void output_test_handle_data(obd_data_t *obd) {
    printf("handling data!\n");
}

void output_test_init(void* data) {
}

void output_test_close(void* data) {
}

static struct output_plugin output_test_plugin = {
    .output_handle_data = output_test_handle_data,
    .output_set_point = 0,
    .output_close = 0
};

struct output_plugin* output_plugin_load(void)
{
    printf("output test doing load.\n");
    return &output_test_plugin;
}
