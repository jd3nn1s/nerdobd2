#include "common.h"

#include <sys/time.h>
#include <stdint.h>
#include <iostream>
#include <ini.h>

#include <easywsclient.hpp>

extern "C" {
    #include "plugins.h"
    struct output_plugin* output_plugin_load(void);
}

using easywsclient::WebSocket;

WebSocket::pointer ws = NULL;
std::string url;

static int ini_handler(void* user, const char* section, const char* name, const char* value) {
    if (std::string(section) != "output_websocket")
        return 1;

    if (std::string(name) != "url")
        return 1;

    url = "ws://";
    url += value;
    return 0;
}

void handle_binary_message(const std::vector<uint8_t>& message) {
    printf("ws: got binary message\n");
}

void output_ws_handle_data(obd_data_t *obd) {
    struct timeval tv;
    double time_val;

    if (!ws || ws->getReadyState() == WebSocket::CLOSED) {
        delete ws;
        ws = WebSocket::from_url(url);
        if (!ws)
            return;
    }

    gettimeofday(&tv, NULL);
    time_val = tv.tv_sec + ((double)tv.tv_usec / 1000000);

    // We forget endianess exists...
    std::vector<uint8_t> buf;
    uint8_t* start = reinterpret_cast<uint8_t*>(obd);
    buf.insert(buf.end(), start, start + sizeof(obd_data_t));
    ws->sendBinary(buf);

    ws->poll();
    ws->dispatchBinary(handle_binary_message);
}

void output_ws_close() {
    if (ws) {
        delete ws;
        ws = 0;
    }
}

static struct output_plugin output_ws_plugin = {
    .handle_data = output_ws_handle_data,
    .set_point = 0,
    .close = output_ws_close
};

struct output_plugin* output_plugin_load(void) {
    bool error = (ini_parse(INI_CONFIG, ini_handler, NULL) < 0);
    if (error || url.length() == 0) {
        std::cerr << "config.ini is missing URL configuration." << std::endl;
        return 0;
    }
    ws = WebSocket::from_url(url);
    if (!ws)
        std::cerr << "Unable to connect websocket, will try later." << std::endl;

    return &output_ws_plugin;
}
