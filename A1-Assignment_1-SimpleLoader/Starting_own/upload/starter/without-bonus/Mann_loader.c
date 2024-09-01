#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *data;
int filesize;
void *virtual_mem;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  munmap(virtual_mem, phdr->p_memsz);
  free(data);
  close(fd);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {

  fd = open(*exe, O_RDONLY);

  // 1. Load entire binary content into the memory from the ELF file.

  if (fd == -1) {
    printf("Error in opening file\n");
    return;
  }

  filesize = lseek(fd, 0, SEEK_END);

  data = malloc(filesize);

  lseek(fd, 0, SEEK_SET);
  read(fd, data, filesize);

  ehdr = (Elf32_Ehdr*) (data);

  phdr = (Elf32_Phdr*) (data + ehdr->e_phoff);

  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c

  int i = 0;
  while ((phdr->p_type != PT_LOAD || phdr->p_vaddr + phdr->p_memsz < ehdr->e_entry) && i < ehdr->e_phnum) {
    phdr = &phdr[1];
    i++;
  }

  if (i == ehdr->e_phnum) {
    printf("Error: couldn't find the entry point\n");
    return;
  }

  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content

  virtual_mem = mmap(NULL, phdr->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);

  if (virtual_mem == MAP_FAILED) {
    printf("Error in allocating memory\n");
    return;
  }

  memcpy(virtual_mem, data + phdr->p_offset, phdr->p_filesz);

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.

  int (*_start)() = (int (*)()) (virtual_mem + ehdr->e_entry - phdr->p_vaddr);

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);
}
int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <elf_file>\n", argv[0]);
    return 1;
  }

  load_and_run_elf(argv[1]);


  loader_cleanup();
  
  return 0;
}