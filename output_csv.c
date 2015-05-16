#include "plugins.h"
#include "common.h"

void output_csv_handle_data(obd_data_t *obd) {
}

void output_csv_close(void* data) {
}

static struct output_plugin output_csv_plugin = {
    .handle_data = output_csv_handle_data,
    .close = output_csv_close
};

struct output_plugin* output_plugin_load(void)
{
    return &output_csv_plugin;
}
