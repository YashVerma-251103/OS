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

struct memory_assigned {
    void *memory;
    struct memory_assigned *next_block;
};

// 
#define null_value NULL
#define true 1
#define false 0


#define page_size 4096

#define st_ma struct memory_assigned

#define elf_magic_number 4
#define phdr Elf32_Phdr
#define ehdr Elf32_Ehdr
#define fault_virtual_address (void *) fault_program_header->p_vaddr
#define fault_segment_size fault_program_header->p_memsz

