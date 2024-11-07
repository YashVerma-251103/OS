// imports
#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

// structurs
struct assigned_memory
{
    // Linked list to keep track of all the memory allocated
    void *memory;
    struct assigned_memory *next;
};

// general definitions
#define true 1
#define false 0
#define success 0
#define failure -1

// value related definitions
#define null_value NULL
#define null_pointer NULL
#define null_char '\0'
#define void_pointer void *
#define char_pointer char *
#define char_array char[]
#define int_pointer int *
#define int_array int[]

// logic related definitions
#define take_input int argc, char **argv
#define argument_count argc
#define argument_values argv

// check related definitions
#define null_check(file) (file == null_value)
#define success_check(file) (file == success)
#define failure_check(file) (file == failure)
#define true_check(file) (file == true)
#define false_check(file) (file == false)

// error related definitions

// file related definitions
#define file_descriptor int
#define elf_header Elf32_Ehdr
#define program_header Elf32_Phdr
#define section_header Elf32_Shdr

// struct related definitions
#define assigned_memory struct assigned_memory
#define assigned_memory_pointer struct assigned_memory *

// Page related definitions
#define page_size 4096

// function declarations
void load_and_run_elf(char_pointer executable);
void loader_cleanup();
