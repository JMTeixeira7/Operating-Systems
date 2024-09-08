#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "messager.h"

void inicialize_path(char path[], int max){
    // Find the size of the string
    int size = 0;
    while (path[size] != '\0' && size < max) {
        ++size;
    }

    // Fill the remaining space with null characters
    for (int i = size; i < max; ++i) {
        path[i] = '\0';
    }
}

void send_msg_char_vector_size(int pipe_fd, const char *vector, size_t size) {

    // Write the vector to the pipe
    ssize_t bytes_written = write(pipe_fd, vector, size);

    if (bytes_written == -1) {
        perror("Error writing to the pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were written
    if ((size_t)bytes_written != size) {
        fprintf(stderr, "Error: Unexpected number of bytes written to the pipe\n");
        exit(EXIT_FAILURE);
    }
}

int send_msg_char(int pipe_fd, const char* str) {
    size_t len = strlen(str);

    // Write to the request pipe
    ssize_t bytes_written = write(pipe_fd, str, len);

    // Check if the expected number of bytes were written
    if (bytes_written == -1 || (size_t)bytes_written != len) {
        perror("Error writing to the pipe");
        return 1;  
    }

    return 0;  
}

void send_msg_unsignedInt(int request_pipe_fd, unsigned int value) {
    // Write the value to the request pipe
    ssize_t bytes_written = write(request_pipe_fd, &value, sizeof(unsigned int));

    if (bytes_written == -1) {
        perror("Error writing to the request pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were written
    if (bytes_written != sizeof(unsigned int)) {
        fprintf(stderr, "Error: Unexpected number of bytes written to the request pipe\n");
        exit(EXIT_FAILURE);
    }
}

void send_msg_Int(int request_pipe_fd, int value) {
    // Write the value to the request pipe
    ssize_t bytes_written = write(request_pipe_fd, &value, sizeof(int));

    if (bytes_written == -1) {
        perror("Error writing to the request pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were written
    if (bytes_written != sizeof(int)) {
        fprintf(stderr, "Error: Unexpected number of bytes written to the request pipe\n");
        exit(EXIT_FAILURE);
    }
}

void send_msg_sizeT(int request_pipe_fd, size_t value){
    // Write the value to the request pipe
    ssize_t bytes_written = write(request_pipe_fd, &value, sizeof(size_t));

    if (bytes_written == -1) {
        perror("Error writing to the request pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were written
    if (bytes_written != sizeof(size_t)) {
        fprintf(stderr, "Error: Unexpected number of bytes written to the request pipe\n");
        exit(EXIT_FAILURE);
    }
}

void send_msg_sizeT_array(int request_pipe_fd, const size_t* values, size_t num_elements) {
    // Calculate the total size of the array
    size_t total_size = num_elements * sizeof(size_t);

    // Write the array to the request pipe
    ssize_t bytes_written = write(request_pipe_fd, values, total_size);

    if (bytes_written == -1) {
        perror("Error writing to the request pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were written
    if ((size_t)bytes_written != total_size) {
        fprintf(stderr, "Error: Unexpected number of bytes written to the request pipe\n");
        exit(EXIT_FAILURE);
    }
}

void send_msg_unsignedInt_array(int request_pipe_fd, const unsigned int* values, size_t num_elements) {
    // Calculate the total size of the array
    size_t total_size = num_elements * sizeof(unsigned int);

    // Write the array to the request pipe
    ssize_t bytes_written = write(request_pipe_fd, values, total_size);

    // Check if the expected number of bytes were written
    if (bytes_written == -1) {
        perror("Error writing to the request pipe");
        exit(EXIT_FAILURE);
    }


}

int read_msg_Int(int response_pipe_fd) {
    int response_value;

    // Read an integer from the response pipe
    ssize_t bytes_read = read(response_pipe_fd, &response_value, sizeof(int));

    if (bytes_read == -1) {
        perror("Error reading from the response pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were read
    if (bytes_read != sizeof(int)) {
        fprintf(stderr, "yError: Unexpected number of bytes read from the pipe\n");
        exit(EXIT_FAILURE);
    }
    return response_value;
}

unsigned int read_msg_unsignedInt(int response_pipe_fd) {
    unsigned int response_value;

    // Read an unsigned integer from the response pipe
    ssize_t bytes_read = read(response_pipe_fd, &response_value, sizeof(unsigned int));

    if (bytes_read == -1) {
        perror("Error reading from the pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were read
    if (bytes_read != sizeof(unsigned int)) {
        fprintf(stderr, "Error: Unexpected number of bytes read from the pipe\n");
        exit(EXIT_FAILURE);
    }

    return response_value;
}


size_t read_msg_sizeT(int response_pipe_fd) {
    size_t response_value;

    // Read a size_t from the response pipe
    ssize_t bytes_read = read(response_pipe_fd, &response_value, sizeof(size_t));

    if (bytes_read == -1) {
        perror("Error reading from the pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were read
    if (bytes_read != sizeof(size_t)) {
        fprintf(stderr, "Error: Unexpected number of bytes read from the pipe\n");
        exit(EXIT_FAILURE);
    }

    return response_value;
}

char read_msg_char(int pipe_fd) {
    char character;

    // Read from the response pipe
    ssize_t bytes_read = read(pipe_fd, &character, sizeof(char));

    // Check if the expected number of bytes were read
    if (bytes_read == -1) {
        perror("Error reading character from pipe");
        return '\0';  
    }
    return character;  
}


size_t* read_msg_sizeT_array(int response_pipe_fd, size_t num_elements) {
    // Initialize variables
    size_t buffer;
    size_t* values = malloc(num_elements * sizeof(size_t));

    if (values == NULL) {
        perror("Error allocating memory for the array");
        exit(EXIT_FAILURE);
    }

    // Read the array from the response pipe until there's nothing left to read
    for (size_t i = 0; i < num_elements; ++i) {
        if (read(response_pipe_fd, &buffer, sizeof(size_t)) <= 0) {
            fprintf(stderr, "Error reading from the response pipe\n");
            free(values);
            exit(EXIT_FAILURE);
        }

        // Assign the read value to the array
        values[i] = buffer;
    }

    return values;
}


unsigned int* read_msg_unsignedInt_array(int response_fd, size_t array_size) {
    // Allocate memory for the array
    unsigned int* array = (unsigned int*)malloc(array_size * sizeof(unsigned int));

    if (array == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Read the array from the response file descriptor
    ssize_t bytes_read = read(response_fd, array, array_size * sizeof(unsigned int));

    if (bytes_read == -1) {
        perror("Error reading from the response pipe");
        free(array);
        exit(EXIT_FAILURE);
    }

    return array;
}

void read_msg_char_vector_size(int pipe_fd, char *buffer, size_t size) {
    ssize_t bytes_read = read(pipe_fd, buffer, size);

    if (bytes_read == -1) {
        perror("Error reading from the pipe");
        exit(EXIT_FAILURE);
    }

    // Check if the expected number of bytes were read
    if ((size_t)bytes_read != size) {
        fprintf(stderr, "Error: Unexpected number of bytes read from the pipe\n");
        exit(EXIT_FAILURE);
    }
}

void writeVectorFile(int fileDescriptor, const unsigned int* vector, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            // Convert the current value to a string
            char buffer[16];
            int charsWritten = snprintf(buffer, sizeof(buffer), "%u", vector[i * cols + j]);

            // Write the formatted value to the file descriptor
            if (write(fileDescriptor, buffer, (size_t)charsWritten) == -1) {
                perror("Error writing to file descriptor");
                return; // Handle the error as needed
            }

            if (j < cols - 1) {
                if (write(fileDescriptor, " ", 1) == -1) {
                    perror("Error writing to file descriptor");
                    return; // Handle the error as needed
                }
            }
        }

        if (write(fileDescriptor, "\n", 1) == -1) {
            perror("Error writing to file descriptor");
            return; // Handle the error as needed
        }
    }
}

void writeEventsToFile(int out_fd, unsigned int* events, size_t num_events) {
    if (num_events == 0) {
        // No events, write "No Events" to the file
        const char* noEventsMsg = "No Events\n";
        write(out_fd, noEventsMsg, strlen(noEventsMsg));
    } else {
        // Write each event to the file
        for (size_t i = 0; i < num_events; ++i) {
            const char* eventPrefix = "Event: ";
            write(out_fd, eventPrefix, strlen(eventPrefix));

            // Write the event number
            char eventNumStr[10]; 
            snprintf(eventNumStr, sizeof(eventNumStr), "%u", events[i]);
            write(out_fd, eventNumStr, strlen(eventNumStr));

            // Write newline character
            const char* newline = "\n";
            write(out_fd, newline, strlen(newline));
        }
    }
}