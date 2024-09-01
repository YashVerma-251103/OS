#include "loader.h"
#include <stdbool.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void *memory_allocated;
void *data_ptr;
int file_size;

void loader_cleanup()
{
	munmap(memory_allocated, phdr->p_memsz);
	free(data_ptr);
	close(fd);
}
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
		// phdr = (Elf32_Phdr *)((char *)phdr + (ehdr->e_phentsize));
		phdr++;
		seg_index++;
	}
	if (seg_index == ehdr->e_phnum)
	{
		printf("Entry Point 404!\n");
		return;
	}

	// 3. Allocate memory of the size "p_memsz" using mmap function
	//    and then copy the segment content
	memory_allocated = mmap(NULL, (phdr->p_memsz), (PROT_READ | PROT_WRITE | PROT_EXEC), (MAP_ANONYMOUS | MAP_PRIVATE), 0, 0);
	if (memory_allocated == MAP_FAILED)
	{
		printf("Allocation o0f memory failed!\n");
		return;
	}
	memcpy(memory_allocated, data_ptr + phdr->p_offset, phdr->p_filesz);

	// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
	// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
	int (*_start)() = (int (*)())((memory_allocated) + ((ehdr->e_entry) - (phdr->p_vaddr)));

	// 6. Call the "_start" method and print the value returned from the "_start"
	printf("User _start return value = %d\n", _start());
}

// Checks for ELF File
bool null_file_check(int file)
{
	if (file == -1)
	{
		printf("ELF file not found!\n");
		return false;
	}
	return true;
}
bool file_validity_check(Elf32_Ehdr e_hdr)
{
	if ((memcmp((e_hdr.e_ident), ELFMAG, SELFMAG)) != 0)
	{
		printf("Not a valid ELF File!\n");
		return false;
	}
	return true;
}
bool executable_file_check(Elf32_Ehdr e_hdr)
{
	if ((e_hdr.e_type) != (ET_EXEC))
	{
		printf("ELF file not executable!\n");
		return false;
	}
	return true;
}
bool section_header_check(Elf32_Ehdr e_hdr)
{
	// SH Number Check
	if (!(0 < (e_hdr.e_shnum) <= (SHN_LORESERVE)))
	{
		printf("Invalid number of Section Headers!\n");
		return false;
	}

	// SH Size Check
	if ((e_hdr.e_shentsize) != (sizeof(Elf32_Shdr)))
	{
		printf("Invalid Size of Section Header! \n");
		return false;
	}
	return true;
}
bool program_header_check(Elf32_Ehdr e_hdr)
{
	// PH Offset Check
	if (!((e_hdr.e_phoff) < ((e_hdr.e_phnum) * (e_hdr.e_phentsize))))
	{
		printf("Invalid Offset for Program Header!\n");
		return false;
	}

	// PH Number Check
	if (!(0 < (e_hdr.e_phnum) <= (PN_XNUM)))
	{
		printf("Invalid number of Program Headers!\n");
		return false;
	}

	// PH Size Check
	if ((e_hdr.e_phentsize) != (sizeof(Elf32_Phdr)))
	{
		printf("Invalid size of Program Header!\n");
		return false;
	}

	return true;
}
bool elf_file_check(int e_file, Elf32_Ehdr e_hdr)
{
	bool file_check = null_file_check(e_file);
	if (file_check)
	{
		file_check = file_validity_check(e_hdr);
		if (!(file_check))
			return false;
		file_check = executable_file_check(e_hdr);
		if (!(file_check))
			return false;
		file_check = section_header_check(e_hdr);
		if (!(file_check))
			return false;
		file_check = program_header_check(e_hdr);
		if (!(file_check))
			return false;

		return true;
	}
	return false;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <ELF Executables>\n", argv[0]);
		exit(1);
	}

	int e_file = open(argv[1], O_RDONLY);
	Elf32_Ehdr e_hdr;
	read(e_file, (&e_hdr), (sizeof(Elf32_Ehdr)));

	// 1. carry out necessary checks on the input ELF file
	if ((elf_file_check(e_file, e_hdr)))
	{
		// 2. passing it to the loader for carrying out the loading/execution
		load_and_run_elf(argv);

		// 3. invoke the cleanup routine inside the loader
		loader_cleanup();
	}
	close(e_file);
	return 0;
}
