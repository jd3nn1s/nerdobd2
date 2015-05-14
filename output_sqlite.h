#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// for copying file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// sqlite database
#include <sqlite3.h>

// include common defines
#include "common.h"
#include "db_common.h"
#include "plugins.h"
#include "config.h"
