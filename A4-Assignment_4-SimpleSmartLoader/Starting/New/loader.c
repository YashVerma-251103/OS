// imports
#include "loader.h"

// global variables for loader
signal_action sig_act;
elf_header_pointer comman_elf_header;
program_header_pointer comman_program_header;
file_descriptor comman_elf_file;
uintptr_t entry_point;

assigned_memory_pointer memory_list;

fragmentation fragmentations = 0;
page faults = 0;
page allocations = 0;



// segmentation fault handler
static signal_handler segmentation_handler(signal_number signal, siginfo_pointer signal_information, void_pointer context )
{
    increment_page_faults;

    void_pointer fault_address = signal_information->si_addr;
    void_pointer alligned_fault_address = (void_pointer)((uintptr_t)fault_address & ~(page_size - 1));

    program_header_pointer fault_segment;
    index offset_in_segment;
    boolean segment_found = false;
    bytes bytes_to_copy = 0;
    bytes file_offset = 0;
    bytes offset = 0;



    for (offset_in_segment = 0; offset_in_segment < comman_elf_header->e_phnum; offset_in_segment++)
    {
        fault_segment = &comman_program_header[offset_in_segment];
        if (((void_pointer)(fault_segment->p_vaddr) <= fault_address) && (fault_address < ((void_pointer)(fault_segment->p_vaddr) + fault_segment->p_memsz)))
        // void_pointer segment_start = (void_pointer)comman_program_header[offset_in_segment].p_vaddr;
        // void_pointer segment_end = ((void_pointer)(comman_program_header[offset_in_segment].p_vaddr) + comman_program_header[offset_in_segment].p_memsz);
        // if ((fault_address >= segment_start) && (fault_address < segment_end))
        {
            // fault_segment = &comman_program_header[offset_in_segment];

            // offset = (bytes)(fault_address - segment_start);
            // offset = (bytes)(fault_address - (void_pointer)(fault_segment->p_vaddr));
            // bytes_to_copy = fault_segment->p_memsz - offset;

            segment_found = true;
            break;
        }      
    }

    if (false_check(segment_found))
    {
        fprintf(stderr, "\nCould not find the segment for the fault address: %p\n", fault_address);
        exit(EXIT_FAILURE);
    }


    bytes_to_copy = (fault_segment->p_offset/page_size) * page_size;
    void_pointer fault_page_memory = mmap(alligned_fault_address, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, comman_elf_file, bytes_to_copy);

    if (fault_page_memory == MAP_FAILED)
    {
        perror("Could not map the memory (segmentation_handler)");
        exit(EXIT_FAILURE);
    }

    print_segment_info(fault_segment, alligned_fault_address, bytes_to_copy);

    page page_number = ((uintptr_t)alligned_fault_address - fault_segment->p_vaddr) / page_size;

    if (page_number == (fault_segment->p_memsz / page_size))
    {
        bytes last_page_used_bytes = fault_segment->p_memsz % page_size;
        if (last_page_used_bytes > 0)
        {
            fragmentations += page_size - last_page_used_bytes;
        }
    }

    increment_page_allocations;

    memory_list = allocate_memory(memory_list, fault_page_memory);
    
    

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
void load_and_run_elf(string file_path)
// void load_and_run_elf(char_pointer file_path)
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
    sig_act.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_sigaction = segmentation_handler;

    if (failure_check(sigaction(SIGSEGV, &sig_act, NULL)))
    {
        perror("sigaction failed! (main)");
        exit(EXIT_FAILURE);
    }

    // const char_pointer elf_path = argument_values[1];
    const string elf_path = argument_values[1];
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

    print_results(faults, allocations, fragmentations);


    return EXIT_SUCCESS;
}