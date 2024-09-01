#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *data;
int filesize;
void *virtual_mem;

void loader_cleanup()
// release memory and other cleanups
{
	munmap(virtual_mem, phdr->p_memsz);
	free(data);
	close(fd);
}

void load_and_run_elf(char **exe)
// Load and run the ELF executable file
{
	const char *target_file_pointer = exe[1];
	fd = open(target_file_pointer, O_RDONLY);

	// 1. Load entire binary content into the memory from the ELF file.
	if (fd == -1)
	{
		// open will return -1 if file is not found or there is some error in opening the file
		printf("Error in opening file\n");
		return;
	}
	filesize = lseek(fd, 0, SEEK_END);
	data = malloc(filesize);
	lseek(fd, 0, SEEK_SET);
	read(fd, data, filesize);
	ehdr = (Elf32_Ehdr *)(data);
	phdr = (Elf32_Phdr *)(data + ehdr->e_phoff);

	// 2. Iterate through the PHDR table and find the section of PT_LOAD type that contains the address of the entrypoint method in fib.c
	int i = 0;
	while (((phdr->p_type != PT_LOAD) || (((phdr->p_vaddr) + (phdr->p_memsz)) < (ehdr->e_entry))) && (i < ehdr->e_phnum))
	{
		phdr = &phdr[1];
		i++;
	}
	if (i == ehdr->e_phnum)
	{
		printf("Error: couldn't find the entry point\n");
		return;
	}

	// 3. Allocate memory of the size "p_memsz" using mmap function and then copy the segment content

	virtual_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (virtual_mem == MAP_FAILED)
	{
		printf("Error in allocating memory\n");
		return;
	}
	memcpy(virtual_mem, data + phdr->p_offset, phdr->p_filesz);

	// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
	// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.

	int (*_start)() = (int (*)())(virtual_mem + ehdr->e_entry - phdr->p_vaddr);

	// 6. Call the "_start" method and print the value returned from the "_start"
	int result = _start();
	printf("User _start return value = %d\n", result);
}

// bool Elf_is_Correct()

int main(int argc, char **argv)
{
	// Need exactly 2 arguments -- one is the executable and the other is the ELF file
	if (argc != 2)
	{
		printf("Usage: %s <ELF Executables>\n", argv[0]);
		exit(1);
	}

	// Open the ELF file and read the ELF header
	const char *elf_p = argv[1];
	int elf_f = open(elf_p, O_RDONLY);
	Elf32_Ehdr elf_h;
	read(elf_f, &elf_h, sizeof(Elf32_Ehdr));

	
	// 1. carry out necessary checks on the input ELF file
	
	if (elf_f == -1)
	// Checking if file is NULL or not
	{
		printf("No elf file found\n");
		exit(1);
	}

	if (memcmp(elf_h.e_ident, ELFMAG, SELFMAG) != 0)
	// Checking Magic Number
	{
		printf("Invalid ELF File\n");
		close(elf_f);
		exit(1);
	}

	if (elf_h.e_type != ET_EXEC)
	// Checking the whether it is executable or not
	{
		printf("Not an executable ELF file\n");
		close(elf_f);
		exit(1);
	}
	
	// Checking all properties of Section Headers
	if (elf_h.e_shnum == 0 || elf_h.e_shnum > SHN_LORESERVE)
	{
		printf("Invalid number of section headers\n");
		exit(1);
	}
	if (elf_h.e_shentsize != sizeof(Elf32_Shdr))
	{
		printf("Invalid section header size\n");
		exit(1);
	}

	// Checking all properties of Program Headers
	if (elf_h.e_phoff >= elf_h.e_phnum * elf_h.e_phentsize)
	{
		printf("Invalid Offset for Program Header\n");
		exit(1);
	}
	if (elf_h.e_phnum == 0 || elf_h.e_phnum > PN_XNUM)
	{
		printf("Invalid number of program headers\n");
		exit(1);
	}
	if (elf_h.e_phentsize != sizeof(Elf32_Phdr))
	{
		printf("Invalid program header size\n");
		exit(1);
	}
	close(elf_f);


	// 2. passing it to the loader for carrying out the loading/execution
	load_and_run_elf(argv);

	// 3. invoke the cleanup routine inside the loader
	loader_cleanup();

	return 0;
}
