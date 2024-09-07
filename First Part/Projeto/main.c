#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>


#include "fileProcess.h"
#include "constants.h"
#include "operations.h"
#include "parser.h"


pthread_mutex_t read_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_Mutex = PTHREAD_MUTEX_INITIALIZER;



int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  int openProcess=0, e=0;

  if (argc > 4) {
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }
  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  int max_threads = atoi(argv[3]);
  int max_proc = atoi(argv[2]);
  struct dirent *dp;

  // Open the directory.
  DIR *dirp = opendir(argv[1]);
    if (dirp == NULL) {
        printf("opendir failed");
        return 1;
    }

  // Reads file from directory (if exists)
  while ((dp = readdir(dirp)) != NULL){
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0){
      continue;  
    }

    if (openProcess > max_proc) {
      int status;
      pid_t finishedChild = wait(&status);
      if (finishedChild > 0) {
        // Child process finished, decrement the count
        openProcess--;
      }
    }

    size_t length = strlen(dp->d_name);
    size_t lengthArgv = strlen(argv[1]);


    //checks if file has ".jobs" sufix, if not goes to next file
    int indx4=0;
    int indx0=0;
    int stopPoint=0;
    for(indx4=0;dp->d_name[indx4]!='\0';indx4++){
      if(dp->d_name[indx4]=='.'){
        stopPoint=indx4;
      }
    }
    char *fileSufix = (char *)malloc((size_t)(indx4 - stopPoint + 1));
    indx4=0;
    for(indx4=stopPoint;dp->d_name[indx4]!='\0';indx4++){
      fileSufix[indx0]=dp->d_name[indx4];
      indx0++;
    }
    fileSufix[indx0]='\0';
    char *dirTitle = (char *)malloc((sizeof(char)*6));
    strcpy(dirTitle, ".jobs\0");
    if(strcmp(fileSufix,dirTitle)){
      free(dirTitle);
      free(fileSufix);
      continue;
    }
    free(dirTitle);
    free(fileSufix);
    //confirmed sufix in file name and free()'s executed


    //Creates new processes and detect if it opens correctly
    pid_t childPid = fork();
    if (childPid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    //if process opens correctly open InputFileDescriptor
    if (childPid==0){
      char *fdName = (char *)malloc((sizeof(char))*(length+lengthArgv+2));
      strcpy(fdName, argv[1]);
      strcat(fdName, "/");
      strcat(fdName, dp->d_name);
      int fd=open(fdName, O_RDONLY);
      free(fdName);
      if (fd < 0)
      {
        printf("open error: %s\n", strerror(errno));
        return -1;
      }
      //InputFileDescriptor opened and free()'s executed


      //creates outputFileDescriptor name to open or create 
      char *wdName = (char *)malloc((sizeof(char))*(length+lengthArgv+2));
      strcpy(wdName, argv[1]);
      strcat(wdName, "/");
      size_t i=0;
      for(i=0;dp->d_name[i]!='.' && dp->d_name[i]!='\0';i++);
      char *fileID = (char *)malloc((sizeof(char))*(i+2));
      for(e=0;dp->d_name[e]!='.' && dp->d_name[e]!='\0';e++){
        fileID[e]= dp->d_name[e];
      }
      fileID[e]='\0';

      strcat(wdName, fileID);
      free(fileID);
      strcat(wdName, ".out");
      int wd=open(wdName, O_WRONLY | O_CREAT |O_TRUNC, S_IRUSR | S_IWUSR);
      free(wdName);
      if (wd < 0)
      {
        printf("open error: %s\n", strerror(errno));
        return -1;
      }
      //outputFileDescriptor opened and free()'s executed

      pthread_t threads[max_threads];
      struct ThreadStruct info[max_threads];

      //inicialize threads struct, with respective information
      int indx3=0;
      for(indx3=0;indx3<max_threads;indx3++){
        info[indx3].fd=fd;
        info[indx3].wd=wd;
        info[indx3].threadNumber=indx3;
        info[indx3].waitMassage=0;
        info[indx3].max_threads=max_threads;
      }
      int inc1=max_threads;
      int inc0=0;

      //create threads
      while((inc1+inc0)==max_threads && inc1!=0){
        for (int indx1 = 0; indx1 < max_threads; ++indx1) {
          if (pthread_create(&threads[indx1], NULL, fileProcess, (void *)&info[indx1]) != 0) {
              perror("pthread_create");
              exit(EXIT_FAILURE);
          }
        }
        //reset counters
        inc1=0;
        inc0=0;
        int result=0;
        for (int indx2 = 0; indx2 < max_threads; ++indx2) {
          void* threadReturn;
          //end threads and get their respective return value
          if (pthread_join(threads[indx2], &threadReturn) != 0) {
              perror("pthread_join");
              exit(EXIT_FAILURE);
          }
          //checks if barrier is active, if it is, while() loop repeats
          if(threadReturn!=NULL)
            result = (int)(intptr_t)threadReturn;
          else
            result=0;
          if (result==1){
            inc1++;
          }
          if (result==0){
            inc0++;
          }
        }
        barrierFlag=0;
      }
      //closes files, move to next file of directory
      close(fd);
      close(wd);
      exit(EXIT_SUCCESS);
    }
    else {
      // This is the parent process
      openProcess++;
      if (openProcess >= max_proc) {
          // Wait for any child process to finish
          int status;
          pid_t finishedChild = waitpid(-1, &status, 0);
          printf("Child Process %d finished with status %d\n", finishedChild, status);
          fflush(stdout);
          if (finishedChild > 0) {
              // Child process finished, decrement the count
              openProcess--;
          }
      }
    }
  }
  while (openProcess > 0) {
    int status;
    pid_t finishedChild = waitpid(-1, &status, 0);
    printf("Child Process %d finished with status %d\n", finishedChild, status);
    fflush(stdout);
    if (finishedChild > 0) {
        // Child process finished, decrement the count
        openProcess--;
    }
    if(finishedChild<0){
      break;
    }
  }
  ems_terminate();
  closedir(dirp);
}
