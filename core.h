// TODO: check whether terminal/socket includes can be in the file that needs them only
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>

// include common defines
#include "common.h"
#include "config.h"
#include "plugins.h"

// serial protocol
#include "kw1281.h"

#ifdef BUILD_HTTPD
#   include "httpd.h"
#endif

#ifdef GPSD_FOUND
#   include "gps.h"
#endif

void block_handle_field(int, float);
void output_data(void);
