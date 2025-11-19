#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include "canox.h"

#define MAX_LONG_NAME 255
#define ID_TEST 9

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

void continuous_test_send() {
  struct timespec next;
  clock_gettime(CLOCK_MONOTONIC, &next);    
  srand(time(NULL) ^ getpid() ^ (getpid() << 16));
  int delay_ms = 100;
  while(1){
    next.tv_nsec += delay_ms * 1000 * 1000;
    if (next.tv_nsec >= 1000000000) {
        next.tv_sec += 1;
        next.tv_nsec -= 1000000000;
    }

    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    struct can_frame frame;
    frame.can_id = ID_TEST;
    frame.can_dlc = 5;
    for (int i = 0; i < frame.can_dlc; i++) {
      frame.data[i] = rand() % 10+1;
    }

    write_can_frame(&frame);
  }
}

void attack() {
  if (setup_filter_attack(ID_TEST)) {
    fprintf(stderr, "Error applying filter!\n");
    return;
  }
  struct can_frame frame;
  while (frame.can_id != ID_TEST) {
    read_can_frame(&frame);
  }
  printf("LETTO!\n");
}

int main(int argc, char **argv){
  char interface[MAX_LONG_NAME];
  int num_generators = 1;
 
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
  pid_t test_pid;


  if (initialize_can_interface(interface)) {
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

  test_pid = fork();
  if (test_pid == 0) {
    continuous_test_send();
    return 0;
  }
  
  attack();
  
  for(int i = 0; i<num_generators; i++) {
    kill(children[i], SIGTERM);
    waitpid(children[i], NULL, 0);
  }

  kill(test_pid, SIGTERM);
  waitpid(test_pid, NULL, 0);

  return 0;
}
