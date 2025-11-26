#ifndef CANOX_H_
#define CANOX_H_

#include <linux/can.h>
#include <linux/can/raw.h>


int initialize_can_interface(char *interface_name, char rx);
int write_can_frame(struct can_frame *frame);
int read_can_frame(struct can_frame *frame);
int setup_filter_attack(int id); 
void cleanup();

#endif
