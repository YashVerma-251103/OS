#include "loader.h"
#include <stdbool.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void *memory_allocated;
void *data_ptr;
int file_size;

/*
 * release memory and other cleanups
 */
void loader_cleanup()
{
	munmap(memory_allocated, phdr->p_memsz);
	free(data_ptr);
	close(fd);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **exe)
{
	fd = open(exe[1], O_RDONLY);

	// 1. Load entire binary content into the memory from the ELF file.
	if (fd == -1)
	{
		// open will return -1 if file is not found or there is some error in opening the file
		printf("Error in opening file!\n");
		return;
	}

	file_size = lseek(fd, 0, SEEK_END);
	data_ptr = malloc(file_size);

	lseek(fd, 0, SEEK_SET);
	read(fd, data_ptr, file_size);

	ehdr = (Elf32_Ehdr *)(data_ptr);
	phdr = (Elf32_Phdr *)(data_ptr + ehdr->e_phoff);

	// 2. Iterate through the PHDR table and find the section of PT_LOAD
	//    type that contains the address of the entrypoint method in fib.c
	int seg_index = 0;
	while ((((phdr->p_type) != PT_LOAD) || (((phdr->p_vaddr) + (phdr->p_memsz)) < (ehdr->e_entry))) && (seg_index < ehdr->e_phnum))
	{
		phdr = (Elf32_Phdr *)((char *)phdr + (ehdr->e_phentsize));
		// phdr++;
		seg_index++;
	}
	if (seg_index == ehdr->e_phnum)
	{
		printf("Entry Point missing!\n");
		return;
	}

	// 3. Allocate memory of the size "p_memsz" using mmap function
	//    and then copy the segment content
	memory_allocated = mmap(NULL, (phdr->p_memsz), (PROT_READ | PROT_WRITE | PROT_EXEC), (MAP_ANONYMOUS | MAP_PRIVATE), 0, 0);
	if (memory_allocated == MAP_FAILED)
	{
		printf("Allocation of memory failed!\n");
		return;
	}
	memcpy(memory_allocated, data_ptr + phdr->p_offset, phdr->p_filesz);

	// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
	// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
	int (*_start)() = (int (*)())((memory_allocated) + ((ehdr->e_entry) - (phdr->p_vaddr)));

	// 6. Call the "_start" method and print the value returned from the "_start"
	int result = _start();
	printf("User _start return value = %d\n", result);
}