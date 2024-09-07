#include "fileProcess.h"
#include "operations.h"
#include "parser.h"
#include "constants.h"


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
#include <stdint.h>

//inicialize global variebles
int barrierFlag=0;
int waitFlag=0;
int threadWait=0;

//function for parseInt, return number of digits a number has
unsigned int countDigits(unsigned int number) {
    unsigned int count = 0;

    // Handle the case when the number is zero separately
    if (number == 0) {
        return 1;
    }

    while (number != 0) {
        number /= 10;  // Divide the number by 10
        count++;       // Increment the count for each division
    }

    return count;
}

//thread function
void* fileProcess(void* arg){
  while (1){
        int exitFlag=0, result=0;
        unsigned int event_id, delay, *thread_id=NULL;
        size_t num_rows, num_columns, num_coords;
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

        //Inicialize values of the thread struct
        struct ThreadStruct *info = (struct ThreadStruct *)arg;
        int fd= info->fd;
        int wd= info->wd;
        int CurrentThread= info->threadNumber;

        //locks other threads, so there is no colision of threads while reading the file.
        pthread_mutex_lock(&read_Mutex);
        switch (get_next(fd)){
          case CMD_CREATE:
            // Check if create command is valid.
            if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              pthread_mutex_unlock(&read_Mutex);
              continue;
            }
            pthread_mutex_unlock(&read_Mutex);
            if (ems_create(event_id, num_rows, num_columns)) {
              fprintf(stderr, "Failed to create event\n");
            }
            break;

          case CMD_RESERVE:
            num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);
            // Check if create command is valid.
            if (num_coords == 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              pthread_mutex_unlock(&read_Mutex);
              continue;
            }
            pthread_mutex_unlock(&read_Mutex);
            if (ems_reserve(event_id, num_coords, xs, ys)) {
              fprintf(stderr, "Failed to reserve seats\n");
            }
            break;

          case CMD_SHOW:
            // Check if create command is valid.
            if (parse_show(fd, &event_id) != 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              pthread_mutex_unlock(&read_Mutex);
              continue;
            }
            pthread_mutex_unlock(&read_Mutex);

            if (ems_show(wd,event_id)) {
              fprintf(stderr, "Failed to show event\n");
            }
            break;

          case CMD_LIST_EVENTS:
            pthread_mutex_unlock(&read_Mutex);
            // blocks threads with the command Show from printing at the same time as LIST
            pthread_mutex_lock(&write_Mutex);
            // Check if create command is valid.
            if (ems_list_events(wd)) {
              fprintf(stderr, "Failed to list events\n");
            }
            pthread_mutex_unlock(&write_Mutex);
            break;

          case CMD_WAIT:
            //blocks wait so the first wait is executed before the next wait takes place
            pthread_mutex_lock(&wait_Mutex);
            int l=parse_wait(fd, &delay, thread_id);
            pthread_mutex_unlock(&read_Mutex);
            // Check if create command is valid.
            if(l == -1) {  
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              pthread_mutex_unlock(&wait_Mutex);
              continue;
            }
            //changes flags
            else if(l == 1 && delay>0) {
              waitFlag=1;
              info->waitMassage=1;
              threadWait=(int)(*thread_id);
              break;
            }
            else if(l == 0 && delay>0) {
              waitFlag=2;
              break;
            }
            break;

          case CMD_INVALID:
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            pthread_mutex_unlock(&read_Mutex);
            break;

          case CMD_HELP:
            pthread_mutex_unlock(&read_Mutex);
            printf(
                "Available commands:\n"
                "  CREATE <event_id> <num_rows> <num_columns>\n"
                "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                "  SHOW <event_id>\n"
                "  LIST\n"
                "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
                "  BARRIER\n"                      // Not implemented
                "  HELP\n");
            break;

          case CMD_BARRIER:
            pthread_mutex_unlock(&read_Mutex);
            //changes barrier flag
            barrierFlag=1;
            break;

          case CMD_EMPTY:
            pthread_mutex_unlock(&read_Mutex);
            break;

          case EOC:
            pthread_mutex_unlock(&read_Mutex);
            //changes endOfFile flag
            exitFlag=1;
            break;
        }
        //all threads wait
        if (waitFlag==2){
          ems_wait(delay);
          waitFlag=0;
          pthread_mutex_unlock(&wait_Mutex);
        }
        //specific thread waits
        if(waitFlag==1 && CurrentThread==threadWait){
          ems_wait(delay);
          waitFlag=0;
          pthread_mutex_unlock(&wait_Mutex);
        }
        //threads ends by BARRIER
        if(barrierFlag==1){
          result=1;
          return (void*)(intptr_t)result;
        }
        //thread ends by endOfFile
        if(exitFlag==1){
          exitFlag=0;
          result=0;
          return (void*)(intptr_t)result;
        }
    }
    return NULL;
}