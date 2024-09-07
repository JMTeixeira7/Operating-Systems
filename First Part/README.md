# Operating-Systems Project First Part

This project implements a theater reservation management system. The code is developed entirely in C, with a focus on mastering core concepts such as file and directory I/O operations, parallelism through process and thread management, and other foundational principles.

Within the "Projeto" directory, you will find the complete source code, a Makefile, and a subdirectory named "jobs" which contains the input files Inputs follow a certain format showed in the PDF.



To compile and run the program:

1- Clean previous build artifacts:
  make clean
  
2- Compile the code:
  make
  
3- Execute the program with following input:
  ./ems jobs 10 5 100       (runs program with 10 processes and 5 threads)
