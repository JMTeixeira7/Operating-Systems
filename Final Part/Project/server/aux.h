#ifndef AUX_H
#define AUX_H

#include "common/constants.h"
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_EVENTS 10

//define command values related with op_code from client
enum ExecutionCommand {
  NOEXECUTION,
  SETUP,
  QUIT,
  CREATE,
  RESERVE,
  SHOW,
  LIST_EVENTS,
  CMD_INVALID,
  EOC
};

//defines global mutexes and global flag
extern int readyFlag;
extern pthread_mutex_t mutex;
extern pthread_cond_t condition;

//struct that holds values related to a specific event
typedef struct{
    int event_id;
    size_t cols;
    size_t rows;
}Events;

//struct that contains data about each node of sessionQueue
typedef struct {
    int sessionID;
    char request_pipe[MAX_PIPE_FIFO];
    char response_pipe[MAX_PIPE_FIFO];
    int req_pipe_fd;
    int resp_pipe_fd;
    Events events[MAX_EVENTS];
}SessionID;

typedef struct Node {
    SessionID data;
    struct Node* next;
} Node;

// struct that defines sessionQueue, buffer that holds client setup requests
typedef struct {
    Node* top;
} StackQueue;

//struct sent to thread functions
typedef struct {
    int threadID;
    StackQueue* stack;
} ThreadStack;


void copyOldestSessionQueue(StackQueue* stack, SessionID* target);

void clearStack(StackQueue* stack);

void printStack(StackQueue* stack);

SessionID pop(StackQueue* stack);

void push(StackQueue* stack, SessionID newData);

int isEmpty(StackQueue* stack);

void initializeStack(StackQueue* stack);

void* worker_thread(void* arg);



#endif