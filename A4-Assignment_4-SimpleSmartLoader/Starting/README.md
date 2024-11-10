# SimpleSmartLoader

## Objective
The objective of this project is to implement a SimpleSmartLoader that loads program segments only when needed during execution (lazy loading). This loader works for ELF 32-bit executables without any references to glibc APIs.

## Implementation Details
1. **Lazy Loading**: The loader does not mmap memory upfront for any segment, including the PT_LOAD type segment containing the entry point address.
2. **Segmentation Fault Handling**: The loader attempts to run the `_start` method directly by typecasting the entry point address. This generates a segmentation fault as the memory page does not exist. The loader handles these segmentation faults by allocating memory using `mmap` and loading the appropriate segments lazily.
3. **Page-by-Page Allocation**: Memory allocation is done page-by-page (4KB) instead of one-shot allocation. This ensures that virtual memory for intra-segment space is contiguous, but physical memory may or may not be contiguous.
4. **Reporting**: After execution, the loader reports:
    - Total number of page faults
    - Total number of page allocations
    - Total amount of internal fragmentation in KB

## Files
- **loader.c**: Main code file implementing lazy loading.
- **loader.h**: Contains macros, definitions, function declarations, libraries, etc.
- **fib.c**: Test case provided.
- **sum.c**: Test case provided.
- **Makefile**: To compile and run the process.

## Usage
1. **Compile**: Run `make compile` to compile the loader and test cases.
2. **Run**: Run `make run` to execute the loader with the test cases.
3. **Clean**: Run `make clean` to remove compiled files.

## Contribution
- **Member 1**: Implemented the main logic for lazy loading and segmentation fault handling.
- **Member 2**: Worked on page-by-page allocation and reporting of page faults, allocations, and internal fragmentation.
- **Member 3**: Created the Makefile and tested the implementation with provided test cases.

## Repository
The project is saved in a private GitHub repository. You can access it [here](https://github.com/your-repo-link).

## Contact
For any queries, please contact us at [your-email@example.com].
