#include "plugins.h"
#include "common.h"

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>

int cansock = 0;

const static uint32_t CANID_SPEED = 0x103;
const static uint32_t CANID_RPM = 0x104;

void output_canbus_handle_data(obd_data_t *obd) {
  struct can_frame frame = {0};
  frame.can_id = CANID_SPEED;
  frame.can_dlc = sizeof(uint8_t);
  frame.data[0] = (uint8_t)obd->speed;
  int retval = write(cansock, &frame, sizeof(frame));
  if (retval != sizeof(frame)) {
    fprintf(stderr, "error sending CAN frame\n");
  }

  frame.can_id = CANID_RPM;
  frame.can_dlc = sizeof(uint16_t);
  frame.data[0] = ((uint16_t)obd->rpm) >> 8;
  frame.data[1] = ((uint16_t)obd->rpm) & 0xff;
  retval = write(cansock, &frame, sizeof(frame));
  if (retval != sizeof(frame)) {
    fprintf(stderr, "error sending CAN frame\n");
  }
}

void output_canbus_close() {
  close(cansock);
  cansock = 0;
}

static struct output_plugin output_canbus_plugin = {
    .handle_data = output_canbus_handle_data,
    .close = output_canbus_close
};

struct output_plugin* output_plugin_load(void)
{
    cansock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (cansock < 0) {
      perror("cannot open CAN bus socket");
    }
    struct sockaddr_can sockaddr = {0};
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(cansock, SIOCGIFINDEX, &ifr) < 0) {
      perror("failed to get interface index");
    }
    sockaddr.can_family = AF_CAN;
    sockaddr.can_ifindex = ifr.ifr_ifindex;
    if (bind(cansock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
      perror("unable to bind to CAN bus socket");
    }

    return &output_canbus_plugin;
}
