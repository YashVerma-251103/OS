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
#include <signal.h>

// structurs
struct assigned_memory
{
    // Linked list to keep track of all the memory allocated
    void *memory;
    // int chunk_index;
    struct assigned_memory *next;
};

// general definitions
#define true 1
#define false 0

// error related definitions
#define success 0
#define failure -1

// value related definitions
#define null_value NULL
#define null_pointer NULL
#define null_char '\0'
#define void_pointer void *
// #define char_pointer char *
#define string char *
#define char_array char[]
#define int_pointer int *
#define int_array int[]
#define boolean int
#define index int


// logical check macros
#define null_check(file) (file == null_value)
#define success_check(file) (file == success)
#define failure_check(file) (file == failure)
#define true_check(file) (file == true)
#define false_check(file) (file == false)

// argument related definitions
#define take_input int argc, char **argv
#define argument_count argc
#define argument_values argv
#define executable_file_path argument_values[1]

// ELF related definitions
#define elf_header Elf32_Ehdr
#define program_header Elf32_Phdr
#define section_header Elf32_Shdr
#define elf_header_pointer Elf32_Ehdr *
#define program_header_pointer Elf32_Phdr *
#define section_header_pointer Elf32_Shdr *
#define elf_magic_number ELFMAG

/* ELF macros */ 
// #define magic_number_check(elf_hdr) ((success_check(memcmp(elf_hdr->e_ident, elf_magic_number, SELFMAG))))
// #define executable_check(elf_hdr) ((elf_hdr->e_type) == (ET_EXEC))
// // section header checks
// #define section_header_number_check(elf_hdr) (((elf_hdr->e_shnum) > 0) && ((elf_hdr->e_shnum) < (SHN_LORESERVE)))
// #define section_header_size_check(elf_hdr) ((elf_hdr->e_shentsize) == (sizeof(section_header)))
// // program header checks
// #define program_header_offset_check(elf_hdr) ((elf_hdr->e_phoff) < ((elf_hdr->e_phnum) * (elf_hdr->e_phentsize)))
// #define program_header_number_check(elf_hdr) (((elf_hdr->e_phnum) > 0) && ((elf_hdr->e_phnum) <= (PN_XNUM)))
// #define program_header_size_check(elf_hdr) ((elf_hdr->e_phentsize) == (sizeof(program_header)))

#define magic_number_check(elf_hdr) ((success_check(memcmp(elf_hdr.e_ident, elf_magic_number, SELFMAG))))
#define executable_check(elf_hdr) ((elf_hdr.e_type) == (ET_EXEC))
// section header checks
#define section_header_number_check(elf_hdr) (((elf_hdr.e_shnum) > 0) && ((elf_hdr.e_shnum) < (SHN_LORESERVE)))
#define section_header_size_check(elf_hdr) ((elf_hdr.e_shentsize) == (sizeof(section_header)))
// program header checks
#define program_header_offset_check(elf_hdr) ((elf_hdr.e_phoff) < ((elf_hdr.e_phnum) * (elf_hdr.e_phentsize)))
#define program_header_number_check(elf_hdr) (((elf_hdr.e_phnum) > 0) && ((elf_hdr.e_phnum) <= (PN_XNUM)))
#define program_header_size_check(elf_hdr) ((elf_hdr.e_phentsize) == (sizeof(program_header)))


// file related definitions
#define file_descriptor int
#define file_open_flags int

// file related macros
#define open_file(file_path) (open(file_path, O_RDONLY))
#define close_file(file) (close(file))
#define read_file(file, buffer, size) (read(file, buffer, size))
#define file_size(file) (lseek(file, 0, SEEK_END))
#define file_seek(file, offset, reference) (lseek(file, offset, reference))


/* struct related definitions */

// memory related definitions and macros
#define assigned_memory struct assigned_memory
#define assigned_memory_pointer assigned_memory *

// signal signature related definitions and macros
#define siginfo_pointer siginfo_t *
#define signal_action struct sigaction
#define signal_action_pointer signal_action *
#define signal_handler void
#define signal_handler_pointer signal_handler *
#define signal_number int
#define signal_number_pointer int *


// Page related definitions
#define page_size 4096
#define bytes size_t
#define page int
#define fragmentation float
#define increment_page_faults page_faults++
#define increment_page_allocations page_allocations++
#define print_segment_info(phdr, aligned_addr, bytes_to_copy) {printf("\nSegment Info:\n"); printf("Page fault at address %p\n", aligned_addr); printf("Copying %zu bytes from offset %p\n\n", bytes_to_copy, aligned_addr);}
#define print_results(faults, allocations, fragmentations) {printf("\nPage Faults: %d\n", faults); printf("Page Allocations: %d\n", allocations); printf("Total Internal Fragmentation: %fKb\n\n", fragmentations/1024);}

// function declarations
// void load_and_run_elf(char_pointer executable);
void load_and_run_elf(string executable);
void loader_cleanup();
boolean check_elf_file(file_descriptor elf_f, elf_header elf_h);
assigned_memory_pointer allocate_memory(assigned_memory_pointer memory_list, void_pointer address);
void free_memory(assigned_memory_pointer memory);
static signal_handler segmentation_handler(signal_number signal, siginfo_pointer signal_information, void_pointer context);


