#include "api.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>  
#include <sys/stat.h>   

#include "../common/messager.h"
#include "../common/constants.h"


int request_fd, response_fd;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  // Create pipes and connect to the server

  unlink(req_pipe_path);
  unlink(resp_pipe_path);

  // Create request pipe
  if (mkfifo(req_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
    return -1;
  }

  // Create respond pipe
  if (mkfifo(resp_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
    return -1;
  }
  
  // Functions used to create the server pipe

  // Open the server pipe for writing only
  int server_pipe = open(server_pipe_path, O_WRONLY);
  if (server_pipe == -1) {
    // Handle error opening the server pipe
    perror("Error opening server pipe for writing");
    return -1;
  }
  int op_code=1;
  
  char req_pipe_path_to_serv[MAX_PIPE_FIFO]="";
  strcat(req_pipe_path_to_serv, &req_pipe_path[0]);

  char resp_pipe_path_to_serv[MAX_PIPE_FIFO]="";
  strcat(resp_pipe_path_to_serv, &resp_pipe_path[0]);

  // Connect request and response pipe to the server
  inicialize_path(req_pipe_path_to_serv, MAX_PIPE_FIFO);
  inicialize_path(resp_pipe_path_to_serv, MAX_PIPE_FIFO);

  struct Message message;
  message.op_code = op_code;
  strncpy(message.req_pipe_path, req_pipe_path_to_serv, MAX_PIPE_FIFO);
  strncpy(message.resp_pipe_path, resp_pipe_path_to_serv, MAX_PIPE_FIFO);

  // Send the entire struct as a single message
  write(server_pipe, &message, sizeof(struct Message));

  //Open pipe for writing, wait for someone to read it.
  request_fd = open(req_pipe_path, O_WRONLY);
  if (request_fd == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  //Open pipe for reading, wait for someone to write in it.
  response_fd = open(resp_pipe_path, O_RDONLY);
    if (response_fd == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

  // Atribute session ID
  int sessionID;
  sessionID=read_msg_Int(response_fd);
  if (sessionID==-1) {
    perror("Read from pipe failed");
    return -1;
  }

  return sessionID;
}

int ems_quit(int sessionID, char const* request_path, char const* response_path) { 
  // Close pipes
  int op_code=2;
  send_msg_Int(request_fd, op_code);
  send_msg_Int(request_fd, sessionID);
  char buffer[16];

  ssize_t bytes_read = read(response_fd, buffer, sizeof(buffer));

  if (bytes_read > 0) {
    return -1;
  } else if (bytes_read == 0) {

  } else {
    // Handle possible errors
    if (errno == EPIPE) {
      printf("Error: Write end of the pipe is closed.\n");
    }else 
      return 1;
    
  }
  if (unlink(request_path) == -1) { // Close request pipe
    perror("Error removing request pipe file");
    return 1; 
  }

  if (unlink(response_path) == -1) { // Close response pipe
    perror("Error removing response pipe file");
    return 1; 
  }
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols, int sessionID) {
  // Send create request to the server (through the request pipe) and wait 
  // for the response (through the response pipe)
  int op_code=3;
  send_msg_Int(request_fd, op_code);
  send_msg_Int(request_fd, sessionID);
  send_msg_unsignedInt(request_fd, event_id);
  send_msg_sizeT(request_fd, num_rows);
  send_msg_sizeT(request_fd, num_cols);

  int value=read_msg_Int(response_fd);
  if (value==1)
    return 1;

  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys, int sessionID) {
  // Send reserve request to the server (through the request pipe) and wait for the response 
  // (through the response pipe)
  int op_code=4;
  send_msg_Int(request_fd, op_code);
  send_msg_Int(request_fd, sessionID);
  send_msg_unsignedInt(request_fd, event_id);
  send_msg_sizeT(request_fd, num_seats);
  send_msg_sizeT_array(request_fd, xs, MAX_RESERVATION_SIZE);
  send_msg_sizeT_array(request_fd, ys, MAX_RESERVATION_SIZE);

  int value=read_msg_Int(response_fd);
  if (value==1)
    return 1;
  
  return 0;
}

int ems_show(int out_fd, unsigned int event_id, int sessionID) {
  // Send show request to the server (through the request pipe) and wait for the 
  // response (through the response pipe)
  int op_code=5;
  send_msg_Int(request_fd, op_code);
  send_msg_Int(request_fd, sessionID);
  send_msg_unsignedInt(request_fd, event_id);

  int value=read_msg_Int(response_fd);

  if(value==1)
    return 1;

  size_t num_rows=read_msg_sizeT(response_fd);
  size_t num_cols=read_msg_sizeT(response_fd);
  unsigned int* vector=read_msg_unsignedInt_array(response_fd, num_rows*num_cols);  
  writeVectorFile(out_fd, vector, num_rows, num_cols);
  free(vector);
  return 0;
}

int ems_list_events(int out_fd, int sessionID) {
  // Send list request to the server (through the request pipe) and wait for the 
  // response (through the response pipe)
  int op_code=6;
  send_msg_Int(request_fd, op_code);
  send_msg_Int(request_fd, sessionID);
  
  int value= read_msg_Int(response_fd);
  if (value==1)
    return 1;

  size_t num_events= read_msg_sizeT(response_fd);
  unsigned int* events= read_msg_unsignedInt_array(response_fd, num_events);    
  writeEventsToFile(out_fd, events, num_events); 
  free(events);
  return 0;
}