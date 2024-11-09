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

#define PAGE_SIZE 4096

struct assigned_memory
{
  void *memory;
  struct assigned_memory *next;
};

void load_and_run_elf(char* exe);
// void load_and_run_elf(char** exe);
void loader_cleanup();
