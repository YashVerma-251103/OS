#include "loader.h"
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PAGE_SIZE 4096

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
char *file_data;
int page_fault_count = 0;
int page_allocation_count = 0;
size_t internal_fragmentation = 0;

void loader_cleanup() {
    close(fd);
    free(file_data);
}

int is_valid_address(void* addr, Elf32_Phdr **segment) {   // To  detect if address access fault generated is a segment fault  or a page fault
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            void *seg_start = (void *)phdr[i].p_vaddr;
            void *seg_end = (void *)(phdr[i].p_vaddr + phdr[i].p_memsz);
            if (addr >= seg_start && addr < seg_end) {
                *segment = &phdr[i];
                return 1; //  Return true if the fault is in the PT_LOAD segment ---> indicating a page fault
            }
        }
    }
    return 0; // Return false if the fault is not in the PT_LOAD segment ---> indicating a "real segment fault
}

void copy_segment_data(void *page_start, char *file_data, Elf32_Phdr *phdr) {
    // Calculate the offset within the segment where this page starts
    size_t offset_in_segment = (uintptr_t)page_start - (uintptr_t)phdr->p_vaddr;

    // Check if the offset is beyond the segment's memory size (p_memsz)
    if (offset_in_segment >= phdr->p_memsz) {
        // If the offset is outside the bounds of the segment's memory size,
        // there is no valid data to copy. Exit the function.
        printf("Offset in segment exceeds the segment's memory size. No data to copy.\n");
        return;
    }

    // Set the default number of bytes to copy as the page size (4KB)
    size_t bytes_to_copy = PAGE_SIZE;

    // Check if the offset and the page size extend beyond the segment's file size (p_filesz)
    if (offset_in_segment + PAGE_SIZE > phdr->p_filesz) {
        // If the current offset is within the file size but extends beyond it,
        // adjust the bytes to copy to avoid reading past the file's data.
        if (offset_in_segment < phdr->p_filesz) {
            bytes_to_copy = phdr->p_filesz - offset_in_segment;
        } else {
            // If the offset is already beyond the file size, no data needs to be copied.
            // Only zero-initialize the page.
            bytes_to_copy = 0;
        }
    }

    // Map a new page at the specified page start address with read, write, and execute permissions
    // printf("Mapping page at address %p\n", page_start);
    void *mapped_page = mmap(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    
    // Check if mmap failed and handle the error
    if (mapped_page == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // If there are bytes to copy, copy them from the file data to the mapped memory page
    if (bytes_to_copy > 0) {
        printf("Copying %zu bytes to address %p\n", bytes_to_copy, mapped_page);
        memcpy(mapped_page, file_data + phdr->p_offset + offset_in_segment, bytes_to_copy);
    } else {
        // If there are no bytes to copy, fill the entire page with zeros
        printf("Copying %zu bytes to address %p\n", bytes_to_copy, mapped_page);
        memset(mapped_page, 0, PAGE_SIZE);
        printf("Filling page at address %p with zeros\n", mapped_page);
    }

    // Increment the count of page allocations
    page_allocation_count++;

    // Calculate and accumulate internal fragmentation (unused space within the allocated page)
    internal_fragmentation += PAGE_SIZE - bytes_to_copy;
}



void segfault_handler(int sig, siginfo_t* info, void* context) {
    void* fault_address = info->si_addr;
    Elf32_Phdr *segment = NULL;
    if (is_valid_address(fault_address, &segment)) {
        printf("Page fault at address %p\n", fault_address);
        void *page_start = (void *)((unsigned long)fault_address & ~(0xFFF));
        copy_segment_data(page_start, file_data, segment);
        page_fault_count++;
        // printf("Paging done\n");
        // printf("Page fault handled\n");
    } else {
        fprintf(stderr, "Segmentation fault at invalid address: %p\n", fault_address);
        exit(1);
    }
}

void setup_sigaction() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }
}

void load_and_run_elf(char** argv) {
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    file_data = (char*) malloc(file_size);
    if (file_data == NULL) {
        perror("malloc failed");
        exit(1);
    }

    if (read(fd, file_data, file_size) != file_size) {
        perror("read failed");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)file_data;
    phdr = (Elf32_Phdr *)(file_data + ehdr->e_phoff);

    setup_sigaction();
    // printf("Starting execution\n");

    Elf32_Addr entry_address = ehdr->e_entry;
    printf("Entry address: %p\n", (void*)entry_address);

    int (*_start)() = (int (*)())entry_address;

    int result = _start();
    printf("User _start return value = %d\n", result);

    printf("Total number of page faults: %d\n", page_fault_count);
    printf("Total number of page allocations: %d\n", page_allocation_count);
    printf("Total amount of internal fragmentation: %zu KB\n", internal_fragmentation / 1024);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);

    loader_cleanup();

    return 0;
}