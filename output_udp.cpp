#include "common.h"

#include <sys/time.h>
#include <stdint.h>
#include <iostream>
#include <ini.h>
#include <resolv.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <vector>
#include <netdb.h>

extern "C" {
    #include "plugins.h"
    struct output_plugin* output_plugin_load(void);
}

int port = 2020;
std::string hostname;
int sock = -1;
struct sockaddr_in servaddr = {0};

static int ini_handler(void* user, const char* section, const char* name, const char* value) {
    if (std::string(section) != "output_udp")
        return 1;

    if (std::string(name) == "hostname")
        hostname = value;
    else if (std::string(name) == "port")
        port = std::stoi(value);
    else
        return 1;

    return 0;
}

void output_udp_handle_data(obd_data_t *obd) {

    // We forget endianess exists...
    std::vector<uint8_t> buf;
    uint8_t* start = reinterpret_cast<uint8_t*>(obd);
    // header that indicates type 1, which is telemetry
    buf.push_back(1);
    buf.insert(buf.end(), start, start + sizeof(obd_data_t));
    if (sendto(sock, (void*)&buf[0], buf.size(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        if (errno != EWOULDBLOCK) {
           std::cerr << "could not send telemetry over udp: " << std::strerror(errno) << std::endl;
        }
    }
}

void output_udp_close() {
}

static struct output_plugin output_udp_plugin = {
    output_udp_handle_data,
    0,
    output_udp_close
};

struct output_plugin* output_plugin_load(void) {
    bool error = (ini_parse(INI_CONFIG, ini_handler, NULL) < 0);
    if (error || hostname.length() == 0) {
        std::cerr << "config.ini is missing hostname configuration." << std::endl;
        return 0;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "unable to create udp socket." << std::endl;
        return 0;
    }

    // set low send buffer size to keep latency low when congestion occurs
    int sndbuf_size = sizeof(obd_data_t);	// linux doubles this
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) == -1) {
        std::cerr << "unable to configure send buffer size" << std::endl;
        return 0;
    }
 
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "unable to set socket to non-blocking" << std::endl;
        return 0;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "bind failed" << std::endl;
        return 0;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    struct hostent* hp = gethostbyname(hostname.c_str());
    if (!hp) {
        std::cerr << "failed to lookup hostname " << hostname << ": " << strerror(errno) << std::endl;
        return 0;
    }
    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    return &output_udp_plugin;
}
