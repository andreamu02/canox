#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "canox.h"

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

int s;
struct ifreq ifr;
struct sockaddr_can addr;

int connect_socket(){
  if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0 ) {
    perror("Cannot open the socket");
    return 1;
  }
  return 0;
}

int initialize_can_interface(char *interface_name) {
  if (connect_socket()) {
    return 1;
  }
  strcpy(ifr.ifr_name, interface_name);
  ioctl(s, SIOCGIFINDEX, &ifr);

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind");
    return 1;
  }

  return 0;
}

int write_can_frame(struct can_frame *frame) {
    if (write(s, frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
      perror("Error during write");
      return 1;  
  }
  return 0;
}

int read_can_frame(struct can_frame *frame) {
  int nbytes;
  nbytes = read(s, frame, sizeof(struct can_frame));
  if (nbytes < 0) {
     perror("Read");
     return 1;
  }
  return 0;
}

int setup_filter_attack(int id) {
  struct can_filter rfilter[1];

  rfilter[0].can_id   = id;
  rfilter[0].can_mask = CAN_SFF_MASK;

  if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0) {
    perror("setsockopt CAN_RAW_FILTER");
    return 1;
  }
  return 0;
}
