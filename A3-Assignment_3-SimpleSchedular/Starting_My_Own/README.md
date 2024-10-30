# SimpleScheduler

## Overview

SimpleScheduler is a basic process scheduler implemented in C. It uses shared memory and semaphores to manage processes and their priorities. The scheduler supports commands to submit processes with different priorities and to start the scheduling cycle.

GitHub: 

## Files

- [`start_again.c`](start_again.c): Contains the main logic for the scheduler, including process management, command handling, and shared memory operations.
- [`dummy_main.h`](dummy_main.h): Provides a signal handler and a mechanism to replace the `main` function with `dummy_main` for testing purposes.
- [`Makefile`](Makefile): Contains the build instructions for the project.

## Compilation
- compile it using 

make

## Usage
- To run the scheduler, use the following command:

./schedular <NCPU> <TSLICE>

<NCPU>: Number of CPUs to simulate.
<TSLICE>: Time slice for each process in milliseconds.