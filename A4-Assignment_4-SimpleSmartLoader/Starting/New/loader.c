// imports
#include "loader.h"

// global variables for loader
elf_header_pointer main_elf_hdr;
program_header_pointer main_program_hdr;
file_descriptor elf_file;
uintptr_t entry_point;

void_pointer virtual_memory;
assigned_memory_pointer memory_list = null_value;

page page_faults = 0;
page page_allocations = 0;
fragmentation total_internal_fragmentation = 0;

// segmentation fault handler
static signal_handler segmentation_handler(signal_number signal, siginfo_pointer signal_information, void_pointer context)
{
    increment_page_faults;

    void_pointer fault_adress = (void_pointer)signal_information->si_addr;
    void_pointer aligned_fault_address = (void_pointer)((uintptr_t)fault_adress & ~(page_size - 1));

    program_header_pointer fault_segment;
    index segment_index = 0;
    boolean segment_found = false;
    for (; segment_index < main_elf_hdr->e_phnum; segment_index++)
    {
        fault_segment = &main_program_hdr[segment_index];
        if (((void_pointer)fault_segment->p_vaddr < fault_adress) && ((void_pointer)(fault_segment->p_vaddr) + fault_segment->p_memsz > fault_adress))
        {
            segment_found = true;
            break;
        }
    }
    if false_check (segment_found)
    {
        perror("Segment not found (segmentation_handler)");
        exit(EXIT_FAILURE);
    }

    // mapping the address at which the segmentaion fault occured and copying the data from file to memory

    if ((fault_segment->p_filesz < fault_segment->p_memsz) && (fault_adress >= (void_pointer)(fault_segment->p_vaddr + fault_segment->p_filesz)))
    {
        // .bss section case : uninitialized data accessed
        virtual_memory = mmap(aligned_fault_address, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    else
    {
        bytes segment_offset = fault_segment->p_offset + (aligned_fault_address - (void_pointer)fault_segment->p_vaddr);
        virtual_memory = mmap(aligned_fault_address, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, elf_file, segment_offset);
    }

    if (virtual_memory == MAP_FAILED)
    {
        perror("Could not map memory (segmentation_handler)");
        exit(EXIT_FAILURE);
    }

    memory_list = allocate_memory(memory_list, (void_pointer)virtual_memory);
    
    increment_page_allocations;
    page page_number = (((bytes)aligned_fault_address - fault_segment->p_vaddr) / page_size);

    // calculating internal fragmentation
    if ((fault_segment->p_memsz - (page_number * page_size)) < page_size)
    {
        total_internal_fragmentation += (page_size*(page_number + 1) - fault_segment->p_memsz);
    }
}

// functions for memory management
void free_memory(assigned_memory_pointer memory)
{
    if null_check (memory)
    {
        return;
    }

    free_memory(memory->next);
    if failure_check (munmap(memory->memory, page_size))
    {
        perror("Could not unmap memory (free_memory)");
        exit(EXIT_FAILURE);
    }
    free(memory);
}
assigned_memory_pointer allocate_memory(assigned_memory_pointer memory_list, void_pointer address)
{
    if null_check (memory_list)
    {
        memory_list = (assigned_memory_pointer)malloc(sizeof(assigned_memory));
        memory_list->memory = address;
        memory_list->next = null_value;
        return memory_list;
    }
    memory_list->next = allocate_memory(memory_list->next, address);
    return memory_list;
}

// functions for loader
void loader_cleanup()
{
    if failure_check (close_file(elf_file))
    {
        perror("Could not close file (loader_cleanup)");
        exit(EXIT_FAILURE);
    }

    free(main_elf_hdr);
    free(main_program_hdr);

    free_memory(memory_list);
}
void load_and_run_elf(string file_path)
{
    if null_check (file_path)
    {
        perror("File path is null (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }

    elf_file = open_file(file_path);
    if failure_check (elf_file)
    {
        perror("Could not open file (load_and_run_elf)");
        exit(EXIT_FAILURE);
    }

    // getting the elf header
    main_elf_hdr = (elf_header_pointer)malloc(sizeof(elf_header));
    if failure_check (read_file(elf_file, main_elf_hdr, sizeof(elf_header)))
    {
        perror("Could not read file (load_and_run_elf : elf header)");
        exit(EXIT_FAILURE);
    }

    // getting the program header
    main_program_hdr = (program_header_pointer)malloc((sizeof(program_header) * main_elf_hdr->e_phnum));
    if failure_check (file_seek(elf_file, main_elf_hdr->e_phoff, SEEK_SET))
    {
        perror("Could not seek file (load_and_run_elf : program header)");
        exit(EXIT_FAILURE);
    }
    if failure_check (read_file(elf_file, main_program_hdr, (sizeof(program_header) * main_elf_hdr->e_phnum)))
    {
        perror("Could not read file (load_and_run_elf : program header)");
        exit(EXIT_FAILURE);
    }

    // getting the entry point
    entry_point = main_elf_hdr->e_entry;

    int (*_start)() = (int (*)())((void_pointer)entry_point);

    int return_value = _start();
    printf("User _start return value = %d\n", return_value);
}
boolean check_elf_file(file_descriptor file, elf_header_pointer elf_hdr)
{
    if failure_check(file)
    {
        perror("File is null (check_elf_file)\n");
        return false;
    }

    if (!magic_number_check(elf_hdr))
    {
        printf("Magic number check failed (check_elf_file)\n");
        return false;
    }

    if (!executable_check(elf_hdr))
    {
        printf("File is not an executable (check_elf_file)\n");
        return false;
    }

    if (!section_header_number_check(elf_hdr))
    {
        printf("Invalid section header number (check_elf_file)\n");
        return false;
    }

    if (!section_header_size_check(elf_hdr))
    {
        printf("Invalid section header size (check_elf_file)\n");
        return false;
    }

    if (!program_header_offset_check(elf_hdr))
    {
        printf("Invalid program header offset (check_elf_file)\n");
        return false;
    }

    if (!program_header_number_check(elf_hdr))
    {
        printf("Invalid program header number (check_elf_file)\n");
        return false;
    }

    if (!program_header_size_check(elf_hdr))
    {
        printf("Invalid program header size (check_elf_file)\n");
        return false;
    }

    return true;
}
int main(take_input)
{
    if (argument_count < 2)
    {
        printf("Usage: %s <executable>\n", argument_values[0]);
        exit(EXIT_FAILURE);
    }

    // setting up the signal handler
    signal_action action;
    action.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = segmentation_handler;
    if failure_check (sigaction(SIGSEGV, &action, null_value))
    {
        perror("Could not set signal handler (main : sigaction)");
        exit(EXIT_FAILURE);
    }

    const string file_path = executable_file_path;
    boolean file_check = check_elf_file(elf_file, main_elf_hdr);
    if false_check (file_check)
    {
        perror("File is not an ELF file (main)");
        exit(EXIT_FAILURE);
    }

    load_and_run_elf(executable_file_path);

    loader_cleanup();

    print_results(page_faults, page_allocations, total_internal_fragmentation);

    return EXIT_SUCCESS;
}