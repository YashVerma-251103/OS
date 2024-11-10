# SimpleSmartLoader

## Objective
The SimpleSmartLoader is designed to load program segments only when needed during execution, implementing lazy loading similar to Linux. It works exclusively for ELF 32-bit executables without references to glibc APIs.

## Implementation Details
1. **Lazy Loading**: No segments, including the PT_LOAD type segment containing the entry point address, are mmaped upfront.
2. **Segmentation Fault Handling**: The loader attempts to run the `_start` method directly, causing a segmentation fault due to unallocated memory pages. These faults are treated as page faults.
3. **Memory Allocation**: The loader handles segmentation faults by allocating memory using mmap and loading the appropriate segments lazily. Memory allocation is done in multiples of the page size (4KB).
4. **Dynamic mmap**: The loader dynamically mmap memory and copies the content of a segment during program execution without terminating the program.
5. **Page-by-Page Allocation**: Instead of one-shot allocation, the loader allocates memory page-by-page for a segment. This results in multiple page faults even for a single segment.
6. **Reporting**: After execution, the loader reports:
    - Total number of page faults
    - Total number of page allocations
    - Total amount of internal fragmentation in KB

## Files
- **loader.c**: Main code file implementing lazy loading.
- **loader.h**: Contains macros, definitions, function declarations, libraries, etc.
- **fib.c**: Test case for Fibonacci calculation.
- **sum.c**: Test case for summing an array.
- **Makefile**: To compile and run the process.

## Usage
1. **Compile**: Run `make compile` to compile the loader and test cases.
2. **Run**: Run `make run` to execute the loader with the test cases.
3. **Clean**: Run `make clean` to remove compiled files.
4. **One-Shot**: Run `make` for all the commands in the series.

## Detailed Implementation
`loader.c`
- **Segmentation Fault Handler**: Handles segmentation faults by allocating memory using `mmap` and loading the appropriate segments.
- **Memory Management**: Functions to allocate and free memory.
- **Loader Functions**: These functions run the ELF file and clean up resources.

`loader.h`
- **Macros and Definitions**: General definitions, error-related definitions, value-related definitions, logical check macros, ELF-related definitions, and file-related macros.
- **Function Declarations**: Declarations for functions used in `loader.c`.

`fib.c`
- **Fibonacci Calculation**: A simple recursive function to calculate the 40th Fibonacci number.

`sum.c`
- **Array Summation**: Initializes an array of size 1024 and sums its elements.


## Contribution
- **Yash Verma** - **2023610**

## Repository
The project is saved in a private GitHub repository. You can access it [here]().
