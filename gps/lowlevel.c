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
#include <asm/termios.h>
#include <asm/ioctls.h>
#include <sys/time.h>
#include <errno.h>
#include <poll.h>

#define SKYTRAQ_RESPONSE_ACK                     0x83
#define SKYTRAQ_RESPONSE_NACK                    0x84

#define TIMEOUT  1000l

typedef struct timeval hp_time;

static double elapsed(hp_time *tv) {
    hp_time now;
    double ot = (double) tv->tv_sec  * 1000 +
                (double) tv->tv_usec / 1000;
    double nt;
    gettimeofday(&now, NULL);
    nt = (double) now.tv_sec  * 1000 +
         (double) now.tv_usec / 1000;

    return nt - ot;
}

int read_with_timeout( int fd, void* buffer, unsigned len, unsigned timeout) {
    hp_time start;
    int offset = 0;
    struct pollfd fds[] = {fd, POLLIN | POLLERR, 0};

    while (len > 0) {
        int ret = poll(fds, 1, timeout);
        if (ret == 0)
           return 0;  // timeout
        else if (ret == -1)
           return 0; // some other error

        int bytesRead = read(fd, buffer + offset, len);
        if ( bytesRead < 0 )
            bytesRead = 0;
        len = len - bytesRead;
        offset = offset + bytesRead;

        /* printf("bytes left: %d  time elapsed: %f\n", len, elapsed(&start)
        );*/

    }

    return offset;
}


gbuint8 readByte( int fd) {
    gbuint8 c;
    read_with_timeout( fd, &c, 1,TIMEOUT);
    return c;
}

unsigned int readInteger( int fd ) {
    gbuint8 high, low;

    high = readByte(fd);
    low  = readByte(fd);

    return (high << 8) + low;
}

gbuint8 calculate_skytraq_checksum( SkyTraqPackage* p ) {
    int i;
    gbuint8 cs = 0;
    for ( i = 0; i< p->length; i++ ) {
        cs = cs ^ p->data[i];
    }
    return cs ;
}

int check_skytraq_checksum( SkyTraqPackage* p ) {
    return calculate_skytraq_checksum(p) == p->checksum ;
}

void skytraq_free_package(  SkyTraqPackage* p ) {
    if ( p != NULL ) {
        if ( p->data != NULL ) {
            free(p->data);
        }
        free(p);
    }
}

void skytraq_dump_package( SkyTraqPackage* p ) {
    int i;
    DEBUG("PACKAGE ( length=%d, ", p->length);
    DEBUG("msg-id=0x%02x, ", p->data[0]);
    DEBUG("data=");
    for ( i = 1; i< p->length; i++) {
        DEBUG("%02x ", p->data[i]);
    }
    DEBUG(")\n");
}

int write_large_buffer( int fd, gbuint8* buf, int len) {
    int i, written;
    int sum_written = 0;

    for ( i = 0; i< len;i++) {
        DEBUG("%02x ", buf[i]);
    }

    while ( len > 0 ) {

        gbuint8* data = buf + sum_written;

        written= write(fd, data, len);

        if ( written < 0 ) {
            if ( errno == EAGAIN ) {
                usleep(10000);
                written = 0;
            } else {
                return written;
            }
        }
        len -= written;

        DEBUG(" (%d byte written) ", written);
        sum_written += written;
    }

    return sum_written;
}

int write_buffer(int fd, gbuint8* buf, int len) {

    if ( len > 20 ) {
        return write_large_buffer(fd,buf,len);
    }

    return write(fd, buf, len);
}

void write_skytraq_package( int fd, SkyTraqPackage* p ) {

    gbuint8* data = malloc( 7 + p->length );

    int i;

    data[0] = 0xa0;
    data[1] = 0xa1;
    data[2] =  (p->length>>8)&0xff;
    DEBUG("%x ", data[2]);
    data[3] =  p->length & 0xff;
    for ( i = 0; i < p->length; i++) {
        data[4+i] =  p->data[i];
        DEBUG("%x ", p->data[i]);
    }
    DEBUG("\n");
    data[4+p->length] =  calculate_skytraq_checksum(p);
    data[5+p->length] =  0x0d;
    data[6+p->length] =  0x0a;

    DEBUG(">>> ");
    write_buffer(fd,data,7 + p->length);
    DEBUG("\n");

    free(data);
}

/**
  * Read the next SkyTraq binary package from the input stream. This function does
  * not return before a package has been read. You have to call skytraq_free_package() on
  * the result.
  */
SkyTraqPackage* skytraq_read_next_package( int fd, unsigned timeout ) {
    int len;
    gbuint8 c, lastByte;
    hp_time start;

    DEBUG("skytraq_read_next_package\n");
    gettimeofday(&start,NULL);

    len = read_with_timeout( fd, &c, 1, timeout);
    while ( len > 0 && elapsed(&start) < timeout ) {
        if ( (lastByte == 0xa0) && (c == 0xa1)) {
            SkyTraqPackage* pkg;
            int dataRead;
            gbuint8 end1, end2;

            pkg = malloc( sizeof(SkyTraqPackage));
            pkg->length = readInteger(fd);
            pkg->data = malloc(pkg->length);
            dataRead = 0;
            while ( dataRead < pkg->length) {
                dataRead += read_with_timeout(fd,pkg->data+dataRead,(pkg->length-dataRead),TIMEOUT);
            }

            pkg->checksum = readByte(fd);

            end1 = readByte(fd);
            end2 = readByte(fd);

            if ( end1 == 0x0d && end2 == 0x0a ) {
                /* a valid package */
                if ( check_skytraq_checksum(pkg) ) {
                    return pkg;
                }
            }
        }

        lastByte = c;
        len = read_with_timeout( fd, &c, 1, timeout);
    }

    return NULL;
}


int skytraq_write_package_with_response( int fd, SkyTraqPackage* p, unsigned timeout ) {
    int retries_left = 300;
    gbuint8 request_message_id;

    request_message_id = p->data[0];
    write_skytraq_package(fd,p);

    DEBUG("Waiting for ACK with msg id 0x%02x\n", request_message_id);

    while (retries_left ) {
        write_skytraq_package(fd,p);
        SkyTraqPackage* response = skytraq_read_next_package(fd, timeout);

        if ( response != NULL) {
            skytraq_dump_package(response);

            if ( response->data[0] == SKYTRAQ_RESPONSE_ACK ) {
                /* ACK - Is it a response to my request? */
                DEBUG("got ACK for msg id: 0x%02x\n", response->data[1] );
                if ( response->data[1] == request_message_id ) {
                    skytraq_free_package(response);
                    return ACK;

                }
            } else if ( response->data[0] == SKYTRAQ_RESPONSE_NACK ) {
                /* NACK - Is it a response to my request? */
                DEBUG("got NACK\n");
                if ( response->data[1] == request_message_id ) {
                    skytraq_free_package(response);
                    return NACK;
                }
            }
            skytraq_free_package(response);
        }
        retries_left--;
    }

    return ERROR;
}

int raw(int fd) {
    struct termios2 tio;

    if (ioctl(fd, TCGETS2, &tio) == ERROR)
        return ERROR;

    tio.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON|IXOFF|CRTSCTS);
    tio.c_oflag &= ~(OPOST);

    tio.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);

    tio.c_cflag &= ~(CSIZE|PARENB);
    tio.c_cflag |= CS8|CLOCAL|CREAD;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME]= 0;

    tcflush(fd, TCIFLUSH);
    if (ioctl(fd, TCSETS2, &tio) == ERROR) {
        printf("tcset() failed.\n");
        return ERROR;
    }
    return SUCCESS;
}

SkyTraqPackage* skytraq_new_package( int length ) {
    SkyTraqPackage* request;
    request = malloc( sizeof(SkyTraqPackage));
    request->length = length;
    request->data = malloc(request->length);
    return request;
}


int open_port( char* device) {
    // open non-blocking so that open will not block, then put back into blocking mode
    int fd = open(device, O_RDWR | O_NONBLOCK);
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
 
    if (raw(fd) == ERROR) {
        printf("unable to call raw()\n");
    }
    if (set_port_speed(fd, 230400) == ERROR) {
        printf("unable to set port speed\n");
    }
    return fd;
}

int set_port_speed(int fd,  unsigned speed) {
    struct termios2 tio = {0};

    if (ioctl(fd, TCGETS2, &tio) == ERROR)
        return ERROR;

    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;

    if (ioctl(fd, TCSETS2, &tio) == ERROR)
        return ERROR;
    return (tio.c_ospeed != speed || tio.c_ispeed != speed) ? ERROR : SUCCESS;
}

/**
  * Read a zero-terminated string from the GPS device.
  */
int read_string( int fd, gbuint8* buffer, int max_length, unsigned timeout ) {
    int len, bytes_read = 0;
    gbuint8 c;
    hp_time start;
    gettimeofday(&start,NULL);

    len = read_with_timeout( fd, &c, 1, timeout);
    while ( len > 0 && elapsed(&start) < timeout ) {
        buffer[bytes_read] = c;

        if ( c == 0 ) {
            /* found end of string */
            return bytes_read;
        }

        bytes_read++;

        if ( bytes_read >= max_length ) {
            return bytes_read; /* Did not reach the end of the string within <max_length> bytes. */
        }

        len = read_with_timeout( fd, &c, 1, timeout);
    }
    DEBUG("read_string: timeout\n");
    return -1;
}
