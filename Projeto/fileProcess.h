#ifndef FILEPROCESS_H
#define FILEPROCESS_H


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>

//Defines the different mutexes and global variebles utilized
extern pthread_mutex_t read_Mutex;
extern pthread_mutex_t write_Mutex;
extern pthread_mutex_t wait_Mutex;

// Flags used in the WAIT command.
extern int waitFlag;
extern int threadWait;

//Flag used in BARRIER command
extern int barrierFlag;

//Define the struct used for threads
struct ThreadStruct{
  int fd;
  int wd;
  int threadNumber;
  int waitMassage;
  int max_threads;
};

//function that returns number of digits of a number
unsigned int countDigits(unsigned int number);

//function called by the threads and executes the major part of the program
void* fileProcess(void* arg);

#endif
