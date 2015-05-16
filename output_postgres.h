#include <stdio.h>
#include <string.h>

// postgres database
#include <libpq-fe.h>

// include common defines
#include "common.h"
#include "db_common.h"
#include "plugins.h"
#include "config.h"

json_object* json_get_data();
json_object* json_get_averages();
json_object* json_get_graph_data(const char *, unsigned long int, unsigned long int);
