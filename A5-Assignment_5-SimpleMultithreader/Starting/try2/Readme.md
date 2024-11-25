# Simple Multithreader 

### Members:
- Yash Verma -- 2023610

### Group ID = 110

### GitHub Repo : 
[GitHub_Page](https://github.com/YashVerma-251103/OS/tree/beaf8e286708fd6fc5d29d5dfb1728abc54f2faf/A5-Assignment_5-SimpleMultithreader)

This project demonstrates parallel computing techniques using C++ and pthreads. It includes examples of parallel vector addition and parallel matrix multiplication.

## Project Structure
- `Makefile`: Build instructions for the project.
- `matrix.cpp`: Example of parallel matrix multiplication.
- `simple-multithreader.h`: Header file containing helper functions and structures for parallel computing.
- `vector.cpp`: Example of parallel vector addition.

## Building the Project

To build the project, run the following command:

```
make
```

This will compile the vector and matrix executables.

## Running the Examples
### Parallel Vector Addition
- To run the parallel vector addition example:
```
./vector [numThreads] [size]
```
- numThreads: Number of threads to use (default is 2).
- size: Size of the vectors (default is 48000000).


## Parallel Matrix Multiplication
- To run the parallel matrix multiplication example:
```
./matrix [numThreads] [size]
```
- numThreads: Number of threads to use (default is 2).
- size: Size of the matrices (default is 1024).


