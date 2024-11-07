// imports
#include "loader.h"

#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

// global variables for loader
struct sigaction signal_action;
elf_header_pointer comman_elf_header;
program_header_pointer comman_program_header;
file_descriptor comman_elf_file;
uintptr_t entry_point;

assigned_memory_pointer memory_list;


int fragmentations = 0;
int page_faults = 0;
int page_allocations = 0;

// segmentation fault handler

static void segmentation_handler(int signal, siginfo_pointer signal_information, void_pointer context )
{
    page_faults++;

    void_pointer fault_address = (void_pointer)signal_information->si_addr;

    printf("Segmentation fault at address: %p\n", fault_address);

    void_pointer aligned_fault_address = (void_pointer)((uintptr_t)fault_address & (~(page_size - 1)));

    boolean fault_found = false;
    program_header_pointer fault_program_header;
    int fault_index;
    for (fault_index = 0; fault_index < (comman_elf_header->e_phnum); fault_index++)
    {
        fault_program_header = &comman_program_header[fault_index];

        if (((void_pointer)(fault_program_header->p_vaddr) <= (fault_address)) && (((void_pointer)(fault_program_header->p_vaddr) + fault_program_header->p_memsz) > (fault_address)))
        {
            fault_found = true;
            break;
        }
    }
    if (false_check(fault_found)){
        printf("Segmentation fault not found! (segmentation handler)\n");
        exit(signal);
    }

    
}

// functions for memory management
void free_memory(assigned_memory_pointer memory)
{
    if (null_check(memory))
    {
        return;
    }

    // navigate to the end of the list
    free_memory(memory->next);

    // clear the memory and free it
    if (failure_check(munmap(memory->memory, page_size)))
    {
        perror("Could_not_unmap_memory (free_memory)");
    }
    free(memory);
}

assigned_memory_pointer allocate_memory(assigned_memory_pointer memory_list, void_pointer address)
{
    // if the memory is null, allocate memory
    if (null_check(memory_list))
    {
        memory_list = malloc(sizeof(assigned_memory));
        memory_list->memory = address;
        return memory_list;
    }

    // navigate to the end of the list
    memory_list->next = allocate_memory(memory_list->next, address);
    return memory_list;
}

// functions for loader
void loader_cleanup()
{
    if (failure_check(close_file(comman_elf_file)))
    {
        perror("Error closing file. (loader_cleanup)");
        exit(EXIT_FAILURE);
    }

    free(comman_elf_header);
    free(comman_program_header);

    // free the memory allocated
    free_memory(memory_list);
}

void load_and_run_elf(char_pointer file_path)
{
    // check if the file exists
    if (null_check(file_path))
    {
        printf("No elf file found\n");
        exit(EXIT_FAILURE);
    }

    // open the file
    file_descriptor comman_elf_file = open_file(file_path);
    if (failure_check(comman_elf_file))
    {
        perror("Error opening file. (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }

    // read the ELF header
    comman_elf_header = malloc(sizeof(elf_header));
    if (failure_check(read_file(comman_elf_file, comman_elf_header, sizeof(elf_header))))
    {
        perror("Error reading file. (Elf Header Reading) (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }

    // read the program headers
    comman_program_header = malloc((comman_elf_header->e_phnum) * (sizeof(program_header)));
    if (failure_check(file_seek(comman_elf_file, comman_elf_header->e_phoff, SEEK_SET)))
    {
        perror("Error seeking file. (Program Header Seek) (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }
    if (failure_check((comman_elf_file, comman_program_header, sizeof(program_header))))
    {
        perror("Error reading file. (Program Header Reading) (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }

    // get the entry point
    entry_point = comman_elf_header->e_entry;

    // typecast the entry point and run
    int (*_start)() = (int (*)())((void_pointer)entry_point);

    // call the _start method
    int result = _start();
    printf("User _start return value = %d\n", result);
}

boolean check_elf_file(file_descriptor file, elf_header elf_hdr)
{
    if (null_check(file))
    {
        printf("No elf file found\n");
        return false;
    }
    if (!(magic_number_check(elf_hdr)))
    {
        printf("Invalid ELF File\n");
        return false;
    }

    if (!(executable_check(elf_hdr)))
    {
        printf("Not an executable ELF file\n");
        return false;
    }

    // Checking all properties of Section Headers
    if (!(section_header_number_check(elf_hdr)))
    {
        printf("Invalid number of section headers\n");
        return false;
    }

    if (!(section_header_size_check(elf_hdr)))
    {
        printf("Invalid section header size\n");
        return false;
    }

    // Checking all properties of Program Headers
    if (!(program_header_offset_check(elf_hdr)))
    {
        printf("Invalid Offset for Program Header\n");
        return false;
    }

    if (!(program_header_number_check(elf_hdr)))
    {
        printf("Invalid number of program headers\n");
        return false;
    }

    if (!(program_header_size_check(elf_hdr)))
    {
        printf("Invalid program header size\n");
        return false;
    }

    // all checks passed
    return true;
}

int main(take_input)
{
    if (argument_count != 2)
    {
        printf("Usage: ./loader <executable>\n");
        exit(EXIT_FAILURE);
    }

    // setting up segmentation fault signal handler
    signal_action.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_sigaction = segmentation_handler;

    if (failure_check(sigaction(SIGSEGV, &signal_action, NULL)))
    {
        perror("sigaction failed! (main)");
        exit(EXIT_FAILURE);
    }

    const char_pointer elf_path = argument_values[1];
    file_descriptor elf_file = open_file(elf_path);
    elf_header elf_hdr;
    read_file(elf_file, &elf_hdr, sizeof(elf_header));

    // Checks for the ELF file
    boolean is_valid_elf = check_elf_file(elf_file, elf_hdr);
    if (false_check(is_valid_elf))
    {
        close_file(elf_file);
        exit(EXIT_FAILURE);
    }

    // loading and execution of the ELF file
    load_and_run_elf(executable_file_path);

    // cleanup of the resources
    loader_cleanup();

    close_file(elf_file);

    return EXIT_SUCCESS;
}