#include "loader.h"

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


void print_segment_info(Elf32_Phdr *phdr, void *aligned_addr, size_t bytes_to_copy)
{
  printf("\nSegment Info:\n");
  // printf("Segment type: %d\n", phdr->p_type);
  // printf("Segment offset: %d\n", phdr->p_offset);
  // printf("Segment virtual address: %p\n", (void *)phdr->p_vaddr);
  // printf("Segment physical address: %p\n", (void *)phdr->p_paddr);
  // printf("Segment file size: %d\n", phdr->p_filesz);
  // printf("Segment memory size: %d\n", phdr->p_memsz);
  // printf("Segment flags: %d\n", phdr->p_flags);
  // printf("Segment alignment: %d\n", phdr->p_align);
  // printf("Segment aligned address: %p\n", aligned_addr);
  // printf("Bytes to copy: %ld\n\n", bytes_to_copy);

  printf("Page fault at address %p\n", aligned_addr);
  printf("Copying %zu bytes from offset %p\n", bytes_to_copy, aligned_addr);
}


static void handler(int sig, siginfo_t *info, void *unused)
{
  page_faults++;
  void *addr = (void *)info->si_addr;

  // printf("SEG FAULT at address: %p\n", addr);

  void * aligned_addr = (void *)((uintptr_t)addr & ~(PAGE_SIZE - 1));

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
  // virtual_mem = mmap((void*)addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
  // virtual_mem = mmap(aligned_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);

  size_t bytes_to_copy = 0;

  // if (phdr->p_filesz < phdr->p_memsz && addr >= (void *)(phdr->p_vaddr + phdr->p_filesz))
  // {
  //   virtual_mem = mmap(aligned_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  // }
  // else{
    ///
    // bytes_to_copy = PAGE_SIZE - ((uintptr_t)addr % PAGE_SIZE);
    // virtual_mem = mmap(aligned_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, phdr->p_offset + (aligned_addr - (void *)phdr->p_vaddr));
  // }

    bytes_to_copy = (phdr->p_offset/PAGE_SIZE) *PAGE_SIZE;
    virtual_mem = mmap(aligned_addr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, bytes_to_copy);

  ///
  if (virtual_mem == MAP_FAILED)
  {
    perror("mmap (after the if and else)");
    exit(sig);
  }

  print_segment_info(phdr, aligned_addr, bytes_to_copy);


  // // int page_num = (((size_t)addr - phdr->p_vaddr)/PAGE_SIZE);
  // int page_num = (((size_t)aligned_addr - phdr->p_vaddr) / PAGE_SIZE);

  // // calculate fragmentation
  // fragmentation += (phdr->p_memsz - (page_num*PAGE_SIZE)) >= PAGE_SIZE ? 0 : (PAGE_SIZE*(page_num+1)) - (phdr->p_memsz);
  // page_allocations++;

  int page_num = (((size_t)aligned_addr - phdr->p_vaddr) / PAGE_SIZE);
  if (page_num == (phdr->p_memsz / PAGE_SIZE))
  {
    size_t last_page_used_bytes = phdr->p_memsz % PAGE_SIZE;
    if (last_page_used_bytes > 0)
    {
      fragmentation += PAGE_SIZE - (last_page_used_bytes);
    }
  }
  page_allocations++;


  // add current address to memory struct
  memory = allocate_memory(memory, (void*)virtual_mem);

  if (virtual_mem == MAP_FAILED)
  {
    perror("mmap");
    exit(sig);
  }
}


// static void handler(int sig, siginfo_t *info, void *unused_context)
// {
//   void* fault_address = info->si_addr;
//   Elf32_Phdr* segment = NULL;

//   int valid_segment = 0;

//   for (int i = 0; i < ehdr->e_phnum; i++)
//   {
//     if (phdrs[i].p_type == PT_LOAD)
//     {
//       void * segment_start = (void *)phdrs[i].p_vaddr;
//       void * segment_end = (void *)(phdrs[i].p_vaddr + phdrs[i].p_memsz);
//       if ((fault_address >= segment_start) && (fault_address < segment_end))
//       {
//         segment = &phdrs[i];
//         valid_segment = 1;
//         break;
//       }
//     }
//   }

//   if(valid_segment == 0)
//   {
//     fprintf(stderr,"Segmentation fault at address %p\n", fault_address);
//     exit(1);
//   }

//   printf("Segmentation fault at address %p\n", fault_address);
//   void *page_start = (void *)((unsigned long)fault_address & ~(0xfff));
//   page_faults++;

//   size_t offset_in_segment = (uintptr_t)page_start - (uintptr_t)segment->p_vaddr;

//   if (offset_in_segment >= segment->p_memsz) {
//     // offset is beyond the end of the segment, hence no valid data to copy.
//     printf("no valid data to copy\n");
//     exit(1);
//   }

//   size_t bytes_to_copy = PAGE_SIZE;
//   if (offset_in_segment + PAGE_SIZE > segment->p_filesz) {
//     if (offset_in_segment >= segment->p_filesz) {
//       // offset is beyond the end of the segment, hence no valid data to copy.
//       bytes_to_copy = 0;
//     } else {
//       bytes_to_copy = segment->p_filesz - offset_in_segment;
//     }
//   }

//   void *mapped_page = mmap(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

//   if (mapped_page == MAP_FAILED) {
//     perror("mmap failed!");
//     exit(1);
//   }

//   printf("Copying %zu bytes from offset %zu to address %p\n", bytes_to_copy, offset_in_segment, mapped_page);
//   if (bytes_to_copy > 0) {
//     // void *segment_start = (void *)((uintptr_t)ehdr + segment->p_offset + offset_in_segment);
//     // memcpy(mapped_page, segment_start, bytes_to_copy);
//     memcpy(mapped_page, (void *)((uintptr_t)ehdr + segment->p_offset + offset_in_segment), bytes_to_copy);
//   }
//   else {
//     memset(mapped_page, 0, PAGE_SIZE);
//     printf("Zeroing out the page\n");
//   }

//   page_allocations++;
//   fragmentation += PAGE_SIZE - bytes_to_copy;


// }

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