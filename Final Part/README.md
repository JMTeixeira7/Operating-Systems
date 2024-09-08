
# Operating-Systems Project Final Part

This program serves as the server implementation for the theater reservation management system introduced in the "First Project." It allows multiple clients to interact concurrently with the server. The project is developed entirely in C, emphasizing the use of inter-process communication via pipes, signal handling, mutex synchronization, and building on the foundational concepts established in the first part of the project.

Within the Projeto directory, you will find the complete source code, a Makefile, and two input files with the .jobs suffix, which define the input for each client. The format of these inputs adheres to specifications outlined in the project documentation (PDF).

To compile and run the program:

1- Clean previous build artifacts: 
    make clean

2- Compile the code: 
    make

3- Open new Terminals for Server and for Client

4- Execute the program in the server terminal:
    ./ems ../server_pipe

5- Execute program in the client side:
    ./client ../request_pipe ../response_pipe ../server_pipe ../b.jobs
