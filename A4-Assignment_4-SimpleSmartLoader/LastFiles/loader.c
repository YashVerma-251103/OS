#include "loader.h"

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
#include <errno.h>
#include <bits/libc-header-start.h>

// assign global variables
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdrs;
int fd;
void *virtual_mem;
uintptr_t entry_point;
int page_faults = 0;
int page_allocations = 0;

int fragmentation = 0;

struct assigned_memory *memory;

// for taking note of addresses assigned
struct assigned_memory *allocate_memory(struct assigned_memory *memory, void *address)
{

  if (memory == NULL)
  {
    memory = malloc(sizeof(struct assigned_memory));
    memory->memory = address;
    return memory;
  }

  memory->next = allocate_memory(memory->next, address);
  return memory;
}

// for freeing memory of the assigned memory
void free_memory(struct assigned_memory *memory)
{
  if (memory == NULL)
  {
    return;
  }
  free_memory(memory->next);
  if (munmap(memory->memory, PAGE_SIZE) == -1) perror("munmap");
  free(memory);
}

static void handler(int sig, siginfo_t *info, void *unused)
{
  page_faults++;
  void *addr = (void *)info->si_addr;

  // printf("SEG FAULT at address: %p\n", addr);

  Elf32_Phdr *phdr;
  int i;
  for (i = 0; i < ehdr->e_phnum; i++)
  {
    phdr = &phdrs[i];
    if (((void *)(phdr->p_vaddr) <= addr) && ((void *)(phdr->p_vaddr) + phdr->p_memsz > addr))
    {
      break;
    }
  }
  if (i == ehdr->e_phnum)
  {
    printf("Not found\n");
    exit(sig);
  }

  // map the address at which seg fault occured and copy the data from fd
  virtual_mem = mmap((void*)addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
  int page_num = (((size_t)addr - phdr->p_vaddr)/PAGE_SIZE);

  // calculate fragmentation
  fragmentation += (phdr->p_memsz - (page_num*PAGE_SIZE)) >= PAGE_SIZE ? 0 : (PAGE_SIZE*(page_num+1)) - (phdr->p_memsz);
  page_allocations++;

  // add current address to memory struct
  memory = allocate_memory(memory, (void*)virtual_mem);

  if (virtual_mem == MAP_FAILED)
  {
    perror("mmap");
    exit(sig);
  }
}

/*
 * release memory and other cleanups
 */
void loader_cleanup()
{
  if (close(fd) == -1)
  {
    perror("close");
    exit(1);
  }

  free(ehdr);
  free(phdrs);

  free_memory(memory);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char *exe)
{
  if (exe == NULL)
  {
    printf("Not a valid ELF executabl\n");
    exit(1);
  }

  fd = open(exe, O_RDONLY);
  // 1. Load entire binary content into the memory from the ELF file.

  if (fd == -1)
  {
    printf("Error in opening file\n");
    return;
  }

  ehdr = malloc(sizeof(Elf32_Ehdr));

  // get ELF Header
  if (read(fd, ehdr, sizeof(Elf32_Ehdr)) == -1)
  {
    perror("read");
    exit(1);
  }

  phdrs = malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));

  // get program headers
  if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1) {
    perror("lseek");
    exit(1);
  }

  if (read(fd, phdrs, ehdr->e_phnum * sizeof(Elf32_Phdr)) == -1) {
    perror("read");
    exit(1);
  }

  entry_point = ehdr->e_entry;

  // typecast the address and run
  int (*_start)() = (int (*)())((void *)entry_point);

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }
  struct sigaction sa;

  // seg fault signal handerl
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;

  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    perror("sigaction");

  // 1. carry out necessary checks on the input ELF file
  const char *elf_p = argv[1];
  int elf_f = open(elf_p, O_RDONLY);
  Elf32_Ehdr elf_h;
  read(elf_f, &elf_h, sizeof(Elf32_Ehdr));
  
  // 1. carry out necessary checks on the input ELF file
  // Checking if file is NULL or not
  if (elf_f == -1)
  {
    printf("No elf file found\n");
    exit(1);
  }

  // Checking Magic Number
  if (memcmp(elf_h.e_ident, ELFMAG, SELFMAG) != 0)
  {
    printf("Invalid ELF File\n");
    close(elf_f);
    exit(1);
  }

  // Checking the whether it is executable or not
  if (elf_h.e_type != ET_EXEC)
  {
    printf("Not an executable ELF file\n");
    close(elf_f);
    exit(1);
  }

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

  // Checking all properties of Section Headers
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
  load_and_run_elf(argv[1]);
  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();

  printf("Total page faults: %d\n", page_faults);
  printf("Total page allocations: %d\n", page_allocations);
  printf("Total internal fragmentation: %f Kb\n", (float)fragmentation / 1024.0);

  return 0;
}