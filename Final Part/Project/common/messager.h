#ifndef MESSAGER_H
#define MESSAGER_H

#include <stddef.h>
#include "constants.h"


// Struct to define a message. Has the code of the command and the 
// name of the request and response pipe
struct Message {
    int op_code;
    char req_pipe_path[MAX_PIPE_FIFO];
    char resp_pipe_path[MAX_PIPE_FIFO];
}; 

#define MAX_BUFFER_SIZE 30

// Initiate path with the name given 
void inicialize_path(char path[], int max);

// Fucntions used to write in the request pipe 
void send_msg_char_vector_size(int pipe_fd, const char *vector, size_t size);
int send_msg_char(int pipe_fd, const char* str);
void send_msg_sizeT(int request_pipe_fd, size_t value);
void send_msg_Int(int request_pipe_fd, int value);
void send_msg_unsignedInt(int request_pipe_fd, unsigned int value);
void send_msg_sizeT_array(int request_pipe_fd, const size_t* values, size_t num_elements);
void send_msg_unsignedInt_array(int request_pipe_fd, const unsigned int* values, size_t num_elements);

// Fucntions used to read from the response pipe
int read_msg_Int(int response_pipe_fd);
char read_msg_char(int pipe_fd);
unsigned int read_msg_unsignedInt(int response_pipe_fd);
size_t read_msg_sizeT(int response_pipe_fd);
size_t* read_msg_sizeT_array(int response_pipe_fd, size_t num_elements);
void read_msg_char_vector_size(int pipe_fd, char *buffer, size_t size);
unsigned int* read_msg_unsignedInt_array(int response_fd, size_t array_size);

// Write the content of a vector to a file
void writeVectorFile(int fileDescriptor, const unsigned int* vector, size_t rows, size_t cols);

// Write the events to a file
void writeEventsToFile(int out_fd, unsigned int* events, size_t num_events);


#endif