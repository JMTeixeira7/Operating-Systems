#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>


#include "signals.h"
#include "operations.h"

// Global variable to indicate if SIGUSR1 is received
volatile sig_atomic_t sigusr1_received = 0;
pthread_mutex_t sigusr1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void signal_handler(int sig) {
    if (sig == SIGUSR1) { //mutex lock to switch value of sigusr1
        pthread_mutex_lock(&sigusr1_mutex); 
        sigusr1_received = 1;
        pthread_mutex_unlock(&sigusr1_mutex);
    }
    else{
        pthread_mutex_lock(&print_mutex);
        fprintf(stderr, "Unexpected signal received: %d\n", sig);
        pthread_mutex_unlock(&print_mutex);

    }
}

void perform__signal() {
    if (sigusr1_received) {
        // call funtion in operation.h that does required print
        pthread_mutex_lock(&print_mutex);
        print_event_data();
        pthread_mutex_unlock(&print_mutex);
        sigusr1_received = 0; // Reset the flag
    }
}

