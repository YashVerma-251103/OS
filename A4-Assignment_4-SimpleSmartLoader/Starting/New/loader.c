#include "./loader.h"

//
void virtual_memory;

ehdr elf_header;
phdr *program_headers;

uintptr_t entry_point;

int file_descriptor;

int page_faults = 0;
int page_allocations = 0;

int internal_fragmentation = 0;

st_ma *memory_list;

st_ma *allocate_memory(st_ma *memory_list, void *address)
{
    // adding new memory block to the list
    if (memory_list == null_value)
    {
        memory_list = malloc(sizeof(st_ma));
        memory_list->memory = address;
        return memory_list;
    }

    memory_list->next_block = allocate_memory(memory_list->next_block, address);
    return memory_list;
}

void free_memory(st_ma *memory_list)
{
    // freeing memory of the assigned memory from the end of the memory list.
    if (memory_list == null_value)
    {
        return;
    }

    free_memory(memory_list->next_block);

    // Freeing the memory block.
    if (munmap(memory_list->memory, page_size) == -1)
    {
        perror("Could not unmap memory! (free_memory)");
    }
    free(memory_list);
}

static void segmentation_handler(int signal, siginfo_t *sig_info, void *context)
{
    // handling segmentation faults
    page_faults++;

    void *address = (void *)sig_info->si_addr;

    // finding the program header that caused the segmentation fault
    phdr *fault_program_header;
    int program_header_index = 0;
    int fault_check = false;

    for (; program_header_index < elf_header.e_phnum; program_header_index++)
    {
        fault_program_header = &program_headers[program_header_index];

        if (((fault_virtual_address) <= (address)) && (((fault_virtual_address) + (fault_segment_size)) > (address)))
        {
            fault_check = true;
            break;
        }
    }

    // if the program header is not found
    if (fault_check == false)
    {
        printf("Segmentation fault not found\n");
        exit(signal);
    }

    // found the page with the fault.



    
}
