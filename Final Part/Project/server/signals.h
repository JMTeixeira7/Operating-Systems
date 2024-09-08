#ifndef SIGNALS_H
#define SIGNALS_H
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>


extern pthread_mutex_t sigusr1_mutex;
extern pthread_mutex_t print_mutex;

void signal_handler(int sig);

//calls print_event_data if condition verifies
void perform__signal();

#endif //SERVER_SIGNALS_H