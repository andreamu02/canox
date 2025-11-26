#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "canox.h"
#include <poll.h>
#include <fcntl.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

int s_tx, s_rx;
struct ifreq ifr;
struct sockaddr_can addr;

int connect_socket(char rx){
  if (rx) {
    if ((s_rx = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0 ) {
      perror("Cannot open RX socket");
      return 1;
    }
    /* make RX non-blocking right away */
    int flags = fcntl(s_rx, F_GETFL, 0);
    if (flags == -1) {
      perror("fcntl(F_GETFL) on s_rx");
      close(s_rx);
      s_rx = -1;
      return 1;
    }
    if (fcntl(s_rx, F_SETFL, flags | O_NONBLOCK) == -1) {
      perror("fcntl(F_SETFL,O_NONBLOCK) on s_rx");
      close(s_rx);
      s_rx = -1;
      return 1;
    }
  } else {
    if ((s_tx = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0 ) {
      perror("Cannot open TX socket");
      return 1;
    }
    /* leave TX as blocking unless you explicitly want it non-blocking */
  }
  return 0;
}

int initialize_can_interface(char *interface_name, char rx) {
  if (connect_socket(rx)) {
    return 1;
  }

  strncpy(ifr.ifr_name, interface_name, IFNAMSIZ-1);
  ifr.ifr_name[IFNAMSIZ-1] = '\0';

  /* use either s_rx or s_tx for ioctl; ioctl doesn't need non-blocking flag */
  if (ioctl(rx ? s_rx : s_tx, SIOCGIFINDEX, &ifr) < 0) {
    perror("ioctl SIOCGIFINDEX");
    return 1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(rx ? s_rx : s_tx, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind");
    return 1;
  }

  /* Optional: avoid receiving our own transmitted frames on the RX socket */
  if (rx) {
    int on = 0; /* 0 = don't receive own messages */
    if (setsockopt(s_rx, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &on, sizeof(on)) < 0) {
      /* not fatal, just warn */
      perror("setsockopt CAN_RAW_RECV_OWN_MSGS");
    }
  }

  return 0;
}

int write_can_frame(struct can_frame *frame) {
    if (write(s_tx, frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
      perror("Error during write");
      return 1;  
  }
  return 0;
}

int read_can_frame(struct can_frame *frame) {
  ssize_t nbytes = read(s_rx, frame, sizeof(struct can_frame));
  if (nbytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 2; /* no data available now */
    }
    perror("read");
    return 1;
  }
  if (nbytes != sizeof(struct can_frame)) {
    fprintf(stderr, "Short CAN frame read: %zd bytes\n", nbytes);
    return 1;
  }
  return 0;
}

int setup_filter_attack(int id) {
  struct can_filter rfilter[1];

  rfilter[0].can_id   = id;
  rfilter[0].can_mask = CAN_SFF_MASK;

  if (setsockopt(s_rx, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0) {
    perror("setsockopt CAN_RAW_FILTER");
    return 1;
  }
  return 0;
}


void cleanup() {
    if (s_tx >= 0) close(s_tx);
    if (s_rx >= 0) close(s_rx);
}
