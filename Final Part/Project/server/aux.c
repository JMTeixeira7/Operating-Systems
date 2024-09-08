#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>

//file dependecies
#include "aux.h"
#include "common/messager.h"
#include "common/constants.h"
#include "operations.h"
#include "signals.h"


//funtions to manipulate the sessionQueue, stack


void initializeStack(StackQueue* stack) {
    stack->top = NULL;
}

int isEmpty(StackQueue* stack) {
    return stack->top == NULL;
}

void push(StackQueue* stack, SessionID newData) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        pthread_mutex_lock(&print_mutex);
        perror("Memory allocation failed");
        pthread_mutex_unlock(&print_mutex);
        exit(EXIT_FAILURE);
    }

    newNode->data = newData;
    newNode->next = stack->top;
    stack->top = newNode;
}

//removes oldest element of stack
SessionID pop(StackQueue* stack) {
    if (isEmpty(stack)) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Stack underflow\n");
        pthread_mutex_unlock(&print_mutex);
        exit(EXIT_FAILURE);
    }

    Node* topNode = stack->top;
    SessionID data = topNode->data;

    stack->top = topNode->next;
    free(topNode);

    return data;
}

void clearStack(StackQueue* stack) {
    while (!isEmpty(stack)) {
        pop(stack);
    }
}

//copies the content of last element in stack to a varieble of type SessionID
void copyOldestSessionQueue(StackQueue* stack, SessionID* target) {
    if (isEmpty(stack)) {
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Stack is empty\n");
        pthread_mutex_unlock(&print_mutex);
        exit(EXIT_FAILURE);
    }

    // Copy the content of the oldest node to the target
    *target = stack->top->data;
}

//thread that makes the process of client requests after ems_setup
void* worker_thread(void* arg){

    //creates mask so, SIGURS1 does not interrupt threads process
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    ThreadStack *threadStack = ( ThreadStack *)arg;
    SessionID sessionID;
    int returnValue, op_code, shut=0;
    while(1){      
        returnValue=0;
        pthread_mutex_lock(&mutex);
        while(readyFlag==0){           //thread waits for permission to process the sessison
            pthread_cond_wait(&condition, &mutex);
        }
        readyFlag=0;
        copyOldestSessionQueue(threadStack->stack, &sessionID);
        pop(threadStack->stack);
        sessionID.sessionID=threadStack->threadID;
        pthread_mutex_lock(&print_mutex);
        send_msg_Int(sessionID.resp_pipe_fd, sessionID.sessionID); //return result of ems_setup to client
        pthread_mutex_unlock(&print_mutex);
        if(pthread_mutex_unlock(&mutex)!=0)
            exit(EXIT_FAILURE);
        
        while(1){           //loop where the process of a client is made, if client session ends then loop ends
            //reads command identifier and session_id, usefull for debug
            pthread_mutex_lock(&print_mutex);
            op_code= read_msg_Int(sessionID.req_pipe_fd);
            int session_id= read_msg_Int(sessionID.req_pipe_fd);
            pthread_mutex_unlock(&print_mutex);
            unsigned int event_id;
            switch (op_code){
                case QUIT:
                    shut=1;
                    //closes request and response pipes
                    if (close(sessionID.req_pipe_fd) == -1) {
                        pthread_mutex_lock(&print_mutex);
                        perror("Error closing request pipe");
                        pthread_mutex_unlock(&print_mutex);
                        break;
                    }
                    if (close(sessionID.resp_pipe_fd) == -1) {
                        pthread_mutex_lock(&print_mutex);
                        perror("Error closing request pipe");
                        pthread_mutex_unlock(&print_mutex);
                        break;
                    }
                    break;
                case CREATE:
                    pthread_mutex_lock(&print_mutex);
                    event_id = read_msg_unsignedInt(sessionID.req_pipe_fd);
                    sessionID.events[event_id].rows= read_msg_sizeT(sessionID.req_pipe_fd);
                    sessionID.events[event_id].cols= read_msg_sizeT(sessionID.req_pipe_fd);
                    pthread_mutex_unlock(&print_mutex);
                    //cretaes event with requested size by client
                    if(ems_create(event_id, sessionID.events[event_id].rows, sessionID.events[event_id].cols))
                        returnValue=1;
                    else
                        returnValue=0;
                    //return to client operation result
                    pthread_mutex_lock(&print_mutex);
                    send_msg_Int(sessionID.resp_pipe_fd,returnValue);
                    pthread_mutex_unlock(&print_mutex);

                    break;
                case RESERVE:
                    pthread_mutex_lock(&print_mutex);
                    event_id= read_msg_unsignedInt(sessionID.req_pipe_fd);
                    size_t num_seats= read_msg_sizeT(sessionID.req_pipe_fd);
                    size_t* xs= read_msg_sizeT_array(sessionID.req_pipe_fd, MAX_RESERVATION_SIZE);    
                    size_t* ys= read_msg_sizeT_array(sessionID.req_pipe_fd, MAX_RESERVATION_SIZE);
                    pthread_mutex_unlock(&print_mutex);
                    //reserves seat with required specification by client
                    if(ems_reserve(event_id, num_seats, xs, ys))
                        returnValue=1;
                    else{
                        returnValue=0;
                    }
                    free(xs);
                    free(ys);
                    //return to client operation result
                    pthread_mutex_lock(&print_mutex);
                    send_msg_Int(sessionID.resp_pipe_fd,returnValue);
                    pthread_mutex_unlock(&print_mutex);

                    break;
                case SHOW:
                    pthread_mutex_lock(&print_mutex);
                    event_id= read_msg_unsignedInt(sessionID.req_pipe_fd);
                    pthread_mutex_unlock(&print_mutex);
                    //vector that will hold the seats values, sent to client only after return value
                    unsigned int* vector = (unsigned int*)malloc(sessionID.events[event_id].rows*sessionID.events[event_id].cols * sizeof(unsigned int));
                    //Calculates rows, cols and vector content
                    if(ems_show( event_id, vector, &sessionID.events[event_id].rows, &sessionID.events[event_id].cols)){
                        returnValue=1;
                        pthread_mutex_lock(&print_mutex);
                        send_msg_Int(sessionID.resp_pipe_fd,returnValue);
                        pthread_mutex_unlock(&print_mutex);
                        free(vector);
                        break;
                    }
                    else{
                        returnValue=0;
                        pthread_mutex_lock(&print_mutex);
                        send_msg_Int(sessionID.resp_pipe_fd,returnValue);
                        pthread_mutex_unlock(&print_mutex);
                    }
                    //sends content to client only if operation is successfull
                    pthread_mutex_lock(&print_mutex);
                    send_msg_sizeT(sessionID.resp_pipe_fd, sessionID.events[event_id].rows);
                    send_msg_sizeT(sessionID.resp_pipe_fd, sessionID.events[event_id].cols);
                    send_msg_unsignedInt_array(sessionID.resp_pipe_fd, vector, sessionID.events[event_id].rows*sessionID.events[event_id].cols);
                    pthread_mutex_unlock(&print_mutex);
                    free(vector);
                    break;
                case LIST_EVENTS:
                    //vector that will hold the events, sent to client only after return value
                    unsigned int* events= (unsigned int*)malloc(MAX_EVENTS * sizeof(unsigned int));
                    size_t num_events=0;
                    //calculates events[] and num_events content
                    if(ems_list_events(events, &num_events)){
                        returnValue=1;
                        pthread_mutex_lock(&print_mutex);
                        send_msg_Int(sessionID.resp_pipe_fd, returnValue);
                        pthread_mutex_unlock(&print_mutex);
                        break;
                    }
                    else{
                        returnValue=0;
                        pthread_mutex_lock(&print_mutex);
                        send_msg_Int(sessionID.resp_pipe_fd, returnValue);
                        pthread_mutex_unlock(&print_mutex);
                    }
                    //sends content to client only if operation is successfull
                    pthread_mutex_lock(&print_mutex);
                    send_msg_sizeT(sessionID.resp_pipe_fd, num_events);
                    send_msg_unsignedInt_array(sessionID.resp_pipe_fd, events, num_events);
                    pthread_mutex_unlock(&print_mutex);
                    free(events);
                    break;
            }
            //checks if ems_quit was processed, if so session ends      
            if(shut==1){
                shut=0;
                break;
            }
        }
    }
    return NULL;
}
