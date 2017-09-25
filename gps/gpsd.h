#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <gps.h>

#include "../common.h"
#include "../config.h"

int     gpsd_start(void);
void    gpsd_stop(void);
int     get_gpsd_data(struct gps_fix_t *);
