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
#include "datalogger.h"
#include "lowlevel.h"

#define TIMEOUT  2000l
#define AGPS_UPLOAD_BLOCKSIZE 8192

int skytraq_read_power_mode(int fd) {
    int result = ERROR;
    SkyTraqPackage* request = skytraq_new_package(2);
    request->data[0] = SKYTRAQ_COMMAND_QUERY_POWER_MODE;
    request->data[1] = 1;
    if ( ACK == skytraq_write_package_with_response(fd,request,TIMEOUT)) {
    }
    else
        fprintf(stderr, "gps: Unable to read software version.\n");

    skytraq_free_package(request);

    return result;
}

int skytraq_read_position_rate(int fd) {
    int result = ERROR;
    SkyTraqPackage* request = skytraq_new_package(2);
    request->data[0] = SKYTRAQ_COMMAND_QUERY_POSITION_RATE;
    request->data[1] = 1;
    if ( ACK == skytraq_write_package_with_response(fd,request,TIMEOUT)) {
    }
    else
        fprintf(stderr, "gps: Unable to read software version.\n");

    skytraq_free_package(request);

    return result;
}

int skytraq_read_software_version(int fd) {
    int result = ERROR;
    SkyTraqPackage* request = skytraq_new_package(2);
    request->data[0] = SKYTRAQ_COMMAND_QUERY_SOFTWARE_VERSION;
    request->data[1] = 1;
    if ( ACK == skytraq_write_package_with_response(fd,request,TIMEOUT)) {
        SkyTraqPackage* response = skytraq_read_next_package(fd,TIMEOUT);
        if ( response != NULL) {
            skytraq_dump_package(response);

            printf("kernel version: %d.%d.%d -- ODM version: %d.%d.%d -- revision: 20%02d-%02d-%02d\n",
                   response->data[3],response->data[4],response->data[5],
                   response->data[7],response->data[8],response->data[9],
                   response->data[11],response->data[12],response->data[13]);
            skytraq_free_package(response);
            result = SUCCESS;
        }
    }
    else
        fprintf(stderr, "gps: Unable to read software version.\n");

    skytraq_free_package(request);

    return result;
}

void skytraq_write_datalogger_config( int fd, skytraq_config* config) {
    SkyTraqPackage* request = skytraq_new_package(27);
    request->data[0] = SKYTRAQ_COMMAND_WRITE_CONFIG;
    request->data[1] = (config->max_time>>24) & 0xff;
    request->data[2] = (config->max_time>>16) & 0xff;
    request->data[3] = (config->max_time>>8) & 0xff;
    request->data[4] = config->max_time & 0xff;
    request->data[5] = (config->min_time>>24) & 0xff;
    request->data[6] = (config->min_time>>16) & 0xff;
    request->data[7] = (config->min_time>>8) & 0xff;
    request->data[8] = config->min_time & 0xff;
    request->data[9] = 00;
    request->data[10] = 00;
    request->data[11] = 03;
    request->data[12] = 0xE8;
    request->data[13] = 00;
    request->data[14] = 00;
    request->data[15] = 00;  /* distance HIGH */
    request->data[16] = config->min_distance & 0xff;
    request->data[17] = 00;
    request->data[18] = 00;
    request->data[19] = 03;
    request->data[20] = 0xE8;
    request->data[21] = 00;
    request->data[22] = 00;
    request->data[23] = (config->min_speed>>8) & 0xff;
    request->data[24] = config->min_speed & 0xff;
    request->data[25] = config->datalog_enable;
    request->data[26] = config->log_fifo_mode;
    skytraq_write_package_with_response(fd,request,TIMEOUT);
    skytraq_free_package(request);
}

unsigned uint32_from_buffer( gbuint8* buffer, int offset ) {
    return buffer[offset] | (buffer[offset+1]<<8) |
           (buffer[offset+2]<<16) | (buffer[offset+3]<<24);
}

unsigned uint16_from_buffer( gbuint8* buffer, int offset ) {
    return buffer[offset]| (buffer[offset+1]<<8);
}

int skytraq_read_datalogger_config( int fd, skytraq_config* config ) {
    int result = ERROR;
    SkyTraqPackage* request = skytraq_new_package(1);
    request->data[0] = SKYTRAQ_COMMAND_GET_CONFIG;
    if ( ACK == skytraq_write_package_with_response(fd,request,TIMEOUT)) {
        SkyTraqPackage* response = skytraq_read_next_package(fd,TIMEOUT);
        if ( response != NULL) {
            skytraq_dump_package(response);

            config->log_wr_ptr = uint32_from_buffer(response->data,1);
            config->sectors_left = uint16_from_buffer(response->data,5);
            config->total_sectors = uint16_from_buffer(response->data,7);
            config->max_time = uint32_from_buffer(response->data,9);
            config->min_time  = uint32_from_buffer(response->data,13);
            config->max_distance   = uint32_from_buffer(response->data,17);
            config->min_distance  = uint32_from_buffer(response->data,21);
            config->max_speed = uint32_from_buffer(response->data,25);
            config->min_speed = uint32_from_buffer(response->data,29);
            config->datalog_enable = response->data[33];
            config->log_fifo_mode = response->data[34];

            skytraq_free_package(response);
            result = SUCCESS;
        }
    }

    skytraq_free_package(request);
    return result;
}

void skytraq_clear_datalog( int fd) {
    SkyTraqPackage* request= skytraq_new_package(1);
    request->data[0] = 0x19;
    skytraq_write_package_with_response(fd,request,TIMEOUT);
    skytraq_free_package(request);
}

/**
 * Returns number of bytes read.
 */
int skytraq_read_datalog_sector( int fd, gbuint8 sector, gbuint8* buffer ) {
    if ( buffer != NULL ) {
        SkyTraqPackage* request= skytraq_new_package(2);;
        request->data[0] = 0x1b;
        request->data[1] = sector;
        if ( ACK == skytraq_write_package_with_response(fd,request,TIMEOUT)) {
            int i,len, count = 0;
            gbuint8 c, lastByte1, lastByte2,cs,checksum;

            DEBUG("START READING DATA\n");

            cs = 0;
            len = read_with_timeout( fd, &c, 1,TIMEOUT);
            while ( len > 0 ) {
                if ( (lastByte2 == 'E') && (lastByte1 == 'N') && (c == 'D')) {
                    /* remove last two bytes from checksum */
                    cs = cs ^'N';
                    cs = cs ^'E';

                    DEBUG("DONE\n");
                    break;
                }


                if ( count >=2 ) {
                    buffer[count-2] = lastByte2;
                }

                cs = cs ^ c;

                lastByte2 = lastByte1;
                lastByte1 = c;
                count++;
                len = read_with_timeout( fd, &c, 1,TIMEOUT);
            }


            /* remaining characters after data block */
            for ( i=0; i<10; i++)
                read_with_timeout( fd, &c, 1, TIMEOUT);

            read_with_timeout( fd, &checksum, 1, TIMEOUT);

            for ( i=0; i<5; i++)
                read_with_timeout( fd, &c, 1, TIMEOUT);


            if ( cs == checksum )
                return count-2;
            else {
                fprintf(stderr, "wrong checksum for sector %d\n", sector);
                return -1;
            }
        }

        skytraq_free_package(request);
    }

    return -1;
}

/**
  * Set the speed used on the serial line.
  */
int skytraq_set_serial_speed( int fd, int speed, int permanent) {
    SkyTraqPackage* request= skytraq_new_package(4);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_SERIAL_PORT;
    request->data[1] = 0;
    request->data[2] = speed;
    request->data[3] = permanent & 1;
    if ( ACK != skytraq_write_package_with_response(fd, request,TIMEOUT) ) {
        fprintf( stderr, "setting line speed FAILED\n");
        return 1;
    }
    skytraq_free_package(request);
    return 0;
}
int skytraq_set_power_mode( int fd, int mode, int permanent) {
    SkyTraqPackage* request= skytraq_new_package(3);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_POWER_MODE;
    request->data[1] = mode;
    request->data[2] = permanent & 1;
    skytraq_dump_package(request);
    if ( ACK != skytraq_write_package_with_response(fd, request,TIMEOUT) ) {
        fprintf( stderr, "setting power mode FAILED\n");
        return 1;
    }
    skytraq_free_package(request);
    return 0;
}

int skytraq_set_position_rate( int fd, int rate, int permanent) {
    SkyTraqPackage* request= skytraq_new_package(3);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_POSITION_RATE;
    request->data[1] = rate;
    request->data[2] = permanent & 1;
    skytraq_dump_package(request);
    if ( ACK != skytraq_write_package_with_response(fd, request,TIMEOUT) ) {
        fprintf( stderr, "setting position rate FAILED\n");
        return 1;
    }
    skytraq_free_package(request);
    return 0;
}

/*unsigned baud_rates[] = { 921600, 115200, 9600,57600 ,1200,2400,4800,19200, 38400 };*/
unsigned baud_rates[] = { 921600, 9600,115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200 };

int contains( gbuint8* haystack, int h_length, gbuint8* needle, int n_length) {
    int offset = 0;
    int i;
    while ( offset < h_length-n_length ) {
        int found = 0;
        for ( i= 0; i<n_length ; i++) {
            if ( haystack[offset+i] == needle[i] ) {
                found++;
            }
        }
        if ( found == n_length ) {
            DEBUG("found needle at offset %d\n", offset);
            return 1;
        }
        offset++;
    }

    return 0;
}



int skytraq_determine_speed( int fd) {
    gbuint8 request[] = { 0xA0, 0xA1, 0x00, 0x02, 0x02, 0x01, 0x03, 0x0D, 0x0A };
    gbuint8 expected_response[] = { 0xA0, 0xA1, 0x00, 0x02, 0x83, 0x02, 0x81, 0x0D, 0x0A};
    int buf_size=4000;
    gbuint8* buffer = malloc(buf_size);
    gbuint8 expected_nmea_responce[] = "$GP";
    int nmea_detection_succeded=0;
    int binary_detection_succeded=0;
    int len,i;
    for ( i = 0; i< (sizeof(baud_rates)/sizeof(int)); i++) {
        DEBUG("testing for %d baud-rate\n", baud_rates[i]);
        set_port_speed(fd, baud_rates[i]);
        /* If baud rate is right we will receive some NMEA data first
         * otherwise at least we will let device enough characters
         * to respond to previous requests */
        len = read_with_timeout(fd, buffer, buf_size,200);

#ifdef DEBUG_ALL
        DEBUG("len=%d\n",len);
        if (len < (buf_size-1))
            buffer[len]=0; /* in C strings must be terminated by 0 */
        else
            buffer[buf_size-1]=0; /* we truncate one symbol from the buffer */
        DEBUG("current ascii output: \n");
        DEBUG("received %d bytes\n", len);
        DEBUG("buffer as ascii contains: %s\n", buffer);
#endif
        if ( contains( buffer, len, expected_nmea_responce, 3)) {
            nmea_detection_succeded=1;
            DEBUG("NMEA detection succeeded\n");
            DEBUG("detected speed is %d\n", baud_rates[i]);
        } else {
            /* try binary detection */

            /* send "QUERY SOFTWARE VERSION" command to GPS unit */
            write(fd, request, 9);

            len = read_with_timeout(fd, buffer, buf_size,200);
#ifdef DEBUG_ALL
            DEBUG("len=%d\n",len);
            if (len < (buf_size-1))
                buffer[len]=0; /* in C strings must be terminated by 0 */
            else
                buffer[buf_size-1]=0; /* we truncate one symbol from the buffer */
            DEBUG("after binary request output: \n");
            DEBUG("received %d bytes\n", len);
            DEBUG("buffer as ascii contains: %s\n", buffer);
#endif

            if ( contains( buffer, len, expected_response, 3)) {
                binary_detection_succeded=1;
                DEBUG("binary detection succeeded\n");
                DEBUG("detected speed is %d\n", baud_rates[i]);
            }
        }

        if (binary_detection_succeded || nmea_detection_succeded) {
            free(buffer);
            return baud_rates[i];
        }
    }

    free(buffer);
    set_port_speed(fd, 38400); /* since we unlucky we set port speed to something high again */
    return 0;
}

int skytraq_output_disable( int fd ) {
    int success;
    SkyTraqPackage* request = skytraq_new_package(3);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_MESSAGE_TYPE;
    request->data[1] = 0;
    request->data[2] = 0;  /* 1 = permanent */

    success = skytraq_write_package_with_response(fd,request,TIMEOUT);
    skytraq_free_package(request);
    return success;
}

int skytraq_output_enable_nmea( int fd ) {
    int success;
    SkyTraqPackage* request = skytraq_new_package(3);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_MESSAGE_TYPE;
    request->data[1] = 1;
    request->data[2] = 0;  /* 1 = permanent */

    success = skytraq_write_package_with_response(fd,request,TIMEOUT);
    skytraq_free_package(request);
    return success;
}

int skytraq_output_enable_binary( int fd, int permanent ) {
    int success;
    SkyTraqPackage* request = skytraq_new_package(3);
    request->data[0] = SKYTRAQ_COMMAND_CONFIGURE_MESSAGE_TYPE;
    request->data[1] = 2;
    request->data[2] = permanent & 1;  /* 1 = permanent */

    success = skytraq_write_package_with_response(fd,request,TIMEOUT);
    skytraq_free_package(request);
    return success;
}
