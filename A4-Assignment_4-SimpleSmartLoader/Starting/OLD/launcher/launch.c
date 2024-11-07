#include "../loader/loader.h"
#include <stdbool.h>

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
bool magic_number_check(Elf32_Ehdr e_hdr)
{
	if ((memcmp((e_hdr.e_ident), ELFMAG, 4)) != 0)
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
		file_check = magic_number_check(e_hdr);
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
