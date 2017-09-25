/*

    Control program for SkyTraq GPS data logger.

    Copyright (C) 2008  Jesper Zedlitz, jesper@zedlitz.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

 */

#ifndef datalogger_h
#define datalogger_h

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

enum {ERROR=-1, SUCCESS};
enum {NACK=-1, ACK};

#define gbuint8 unsigned char
#define gbuint32 unsigned long
#define gbuint16 unsigned int

//#define DEBUG_ALL

#ifdef DEBUG_ALL
#undef DEBUG
#define DEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif

#define SKYTRAQ_SPEED_4800      0
#define SKYTRAQ_SPEED_9600      1
#define SKYTRAQ_SPEED_19200     2
#define SKYTRAQ_SPEED_38400     3
#define SKYTRAQ_SPEED_57600     4
#define SKYTRAQ_SPEED_115200    5

#define SKYTRAQ_COMMAND_SYSTEM_RESTART           0x01
#define SKYTRAQ_COMMAND_QUERY_SOFTWARE_VERSION   0x02
#define SKYTRAQ_COMMAND_QUERY_SOFTWARE_CRC       0x03
#define SKYTRAQ_COMMAND_SET_FACTORY_DEFAULTS     0x04
#define SKYTRAQ_COMMAND_CONFIGURE_SERIAL_PORT    0x05
#define SKYTRAQ_COMMAND_CONFIGURE_NMEA_MESSAGE   0x08
#define SKYTRAQ_COMMAND_CONFIGURE_MESSAGE_TYPE   0x09
#define SKYTRAQ_COMMAND_CONFIGURE_POWER_MODE     0x0C
#define SKYTRAQ_COMMAND_QUERY_POSITION_RATE      0x10
#define SKYTRAQ_COMMAND_CONFIGURE_POSITION_RATE  0x0E
#define SKYTRAQ_COMMAND_QUERY_POWER_MODE         0x15
#define SKYTRAQ_COMMAND_GET_CONFIG               0x17
#define SKYTRAQ_COMMAND_WRITE_CONFIG             0x18
#define SKYTRAQ_COMMAND_ERASE                    0x19
#define SKYTRAQ_COMMAND_READ_SECTOR              0x1b
#define SKYTRAQ_COMMAND_GET_EPHERMERIS           0x30
#define SKYTRAQ_COMMAND_SET_EPHEMERIS            0x31
#define SKYTRAQ_COMMAND_READ_AGPS_STATUS         0x34
#define SKYTRAQ_COMMAND_SEND_AGPS_DATA           0x35
#define SKYTRAQ_RESPONSE_SOFTWARE_VERSION        0x80
#define SKYTRAQ_RESPONSE_SOFTWARE_CRC            0x81
#define SKYTRAQ_RESPONSE_ACK                     0x83
#define SKYTRAQ_RESPONSE_NACK                    0x84
#define SKYTRAQ_RESPONSE_POSITION_RATE           0x86
#define SKYTRAQ_RESPONSE_NAVIGATION_DATA         0xA8
#define SKYTRAQ_RESPONSE_EPHEMERIS_DATA          0xB1
#define SKYTRAQ_RESPONSE_POWER_MODE              0xB9

typedef struct SkyTraqPackage {
    gbuint8	length;
    gbuint8* 	data;
    gbuint8 	checksum;
} SkyTraqPackage;

typedef struct skytraq_config {
    gbuint32    log_wr_ptr;
    gbuint16    sectors_left;
    gbuint16    total_sectors;
    gbuint32	max_time;
    gbuint32	min_time;
    gbuint32	max_distance;
    gbuint32	min_distance;
    gbuint32	max_speed;
    gbuint32	min_speed;
    gbuint8	datalog_enable;
    gbuint8	log_fifo_mode;
} skytraq_config;

int skytraq_read_software_version( int fd);
int skytraq_read_position_rate(int fd);
int skytraq_read_datalogger_config( int fd, skytraq_config* config);
int skytraq_read_datalog_sector( int fd, gbuint8 sector, gbuint8* buffer );
void skytraq_clear_datalog( int fd);
void skytraq_write_datalogger_config( int fd, skytraq_config* config);
long process_buffer(const gbuint8* buffer,const  int length,const  long last_timestamp);
int skytraq_determine_speed( int fd) ;
int skytraq_set_serial_speed( int fd, int speed, int permanent);
int skytraq_output_disable( int fd );
int skytraq_output_enable_nmea( int fd );
int skytraq_output_enable_binary( int fd, int permanent );
int skytraq_set_position_rate( int fd, int rate, int permanent );

#endif
