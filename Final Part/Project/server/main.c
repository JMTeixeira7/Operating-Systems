#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

//file dependicies
#include "common/constants.h"
#include "common/io.h"
#include "common/messager.h"
#include "operations.h"
#include "aux.h"
#include "signals.h"

//inicializes both mutex and variable condition
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
int readyFlag = 0; //readyFlag=1 means session is ready to be processed by thread

int main(int argc, char* argv[]){
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  int i=0, launched=0;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  //unlink previous pipe and creates a new one
  unlink(argv[1]);
  mkfifo(argv[1], 0666);

  // Open the server pipe for reading and writing
  int server_pipe = open(argv[1], O_RDWR);

  if (server_pipe == -1) {
    // Handle error opening the server pipe
    perror("Error opening server pipe for reading");
    return 1;
  }
  //Inicializes the buffer that holds the requests from clients
  StackQueue sessionQueue;
  initializeStack(&sessionQueue);

  //Inicializes the vector that contains the sessionQueue and respective threadID
  ThreadStack threadStacks[MAX_SESSION_COUNT];
  for (i = 0; i < MAX_SESSION_COUNT; ++i) {
    threadStacks[i].threadID = i;
    threadStacks[i].stack = &sessionQueue;  // All ThreadStacks point to the same StackQueue
  }
  //registers signal_handler(int sig) as SIGUSR1 handler
  if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
      perror("Error setting up SIGUSR1 handler");
      exit(EXIT_FAILURE);
  }

  //Server loop, each iteration server reads a session request from clients
  int cont=0;
  while (1) {
    if(cont==2)
      raise(SIGUSR1);
    //launches threads, only happens once
    if (launched==0){
      pthread_t threads[MAX_SESSION_COUNT];
      int e=0;
      for (e = 0; e < MAX_SESSION_COUNT; ++e){
        if (pthread_create(&threads[e], NULL, worker_thread, (void*)&threadStacks[e]) != 0) {
          perror("Error creating thread");
          return 1;
        }
      }
      launched=1;
    }

    int op_code;
    SessionID newSession;
    newSession.sessionID=0;
    struct Message received_message;
    // Read the entire struct with a single read call
    ssize_t bytes_read = read(server_pipe, &received_message, sizeof(struct Message));

    if (bytes_read == -1) {
        perror("Error reading from the pipe");

        // Handle the error as needed
    } else if (bytes_read == 0) {
        // End of file, pipe is closed
    } else if (bytes_read != sizeof(struct Message)) {
        fprintf(stderr, "Error: Unexpected number of bytes read from the pipe\n");

        // Handle the error as needed
    } else {
        // Access the fields of the received struct
        op_code = received_message.op_code;
        strncpy(newSession.request_pipe, received_message.req_pipe_path, MAX_PIPE_FIFO);
        strncpy(newSession.response_pipe, received_message.resp_pipe_path, MAX_PIPE_FIFO);
    }
    if(op_code==1){ //if ems_setup is requested by client
      //opens request and response pipes from server side
      newSession.req_pipe_fd = open(newSession.request_pipe, O_RDONLY);
      if (newSession.req_pipe_fd == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      newSession.resp_pipe_fd = open(newSession.response_pipe, O_WRONLY);
      if (newSession.resp_pipe_fd == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      //activates mutex lock, because of sessionQueue insertion
      pthread_mutex_lock(&mutex);   
      push(&sessionQueue, newSession);
      readyFlag=1;          //thread can now take charge of this session
      if (pthread_cond_signal(&condition) != 0){
        perror("Error signaling condition variable");
      }
      if(pthread_mutex_unlock(&mutex) != 0) {
        fprintf(stderr, "Error unlocking mutex for session %d\n", i);
            // Handle the error as needed
      }
    }
    //activates mutex lock to print events data, without interuption
    pthread_mutex_lock(&sigusr1_mutex);
    perform__signal();
    pthread_mutex_unlock(&sigusr1_mutex);
    cont ++;
  }
  //Close Server & terminates server
  close(server_pipe);
  ems_terminate();
}