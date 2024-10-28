#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <bits/sigaction.h>

// logic related constants
#define true 1
#define false 0
#define null_status NULL
#define null_value NULL
#define error_status -1
#define success_status 0
#define zero_value 0

// Shell related constants
#define command_buffer 128
#define stc struct commands

// Structure related constants
#define stp struct process
#define stm struct shm_t

// Importing Required functions
static void child_signal_handler(int signum);
unsigned long elapse_time_only_start(struct timeval *start);

// Constructing Required Structures
struct process
{
    pid_t pid;
    char command[command_buffer];
    int num_slice;
    int wait_time;
    struct timeval last_exec;
    struct process *next;
};
typedef struct shm_t
{
    char command[command_buffer];
    int priority;
    sem_t mutex;
} shm_t;

// Setting up the global variables
sigset_t signal_mask;
shm_t *shm;
int shm_file_descriptor;
int no_of_CPUs;
int time_slice;

// utility functions for shared memory
shm_t *memory_setup();
void memory_cleanup(shm_t *shm);
void child_memory_cleanup(shm_t *shm);

// utility functions for processes
// stp *contruct_process(stp *process_queue, char command[]);
stp *contruct_process(stp *process_queue, char *command);
void sleep_ms(long ms);
void pause_process(stp *process);
void continue_process(stp *process);

// utility functions for process queue
stp *enqueue_process(stp *process_queue, stp *process);
stp *requeue_running_process(stp *process_queue, stp *running_process);
stp *clean_stopped_process(stp *process);

// utility functions for ready and running processes
stp *cycle_processes_in_queue(stp *process_queue);
stp *run_next_process(stp *process_queue);

shm_t *memory_setup()
{
    shm_file_descriptor = shm_open("Vemy_main_sc_shm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_file_descriptor == error_status)
    {
        perror("Could not open the shared memory file.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_file_descriptor, sizeof(shm_t)) == error_status)
    {
        perror("Could not truncate the shared memory file.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    shm_t *new_shm = mmap(null_status, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_file_descriptor, 0);
    if (new_shm == MAP_FAILED)
    {
        perror("Could not map the shared memory file.");
        // exit(0);
        exit(EXIT_FAILURE);
    }

    // initializing the semaphore.
    int sem_init_status = sem_init(&new_shm->mutex, true, 1);
    if (sem_init_status == error_status)
    {
        perror("Could not initialize the semaphore.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    return new_shm;
}
void memory_cleanup(shm_t *shm)
{
    if (sem_destroy(&shm->mutex) == error_status)
    {
        perror("Could not destroy the semaphore.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    if (munmap(shm, sizeof(stm)) == error_status)
    {
        perror("Could not unmap the shared memory.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    if (close(shm_file_descriptor) == error_status)
    {
        perror("Could not close the shared memory file.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    if (shm_unlink("Vemy_main_sc_shm") == error_status)
    {
        perror("Could not unlink the shared memory.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
}
void child_memory_cleanup(shm_t *shm)
{
    if (munmap(shm, sizeof(stm)) == error_status)
    {
        perror("Could not unmap the shared memory. (Child)");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    if (close(shm_file_descriptor) == error_status)
    {
        perror("Could not close the shared memory file. (Child)");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    // exit(0);
    exit(EXIT_SUCCESS);
}

stp *contruct_process(stp *process_queue, char *command)
{
    if (process_queue == null_status)
    {
        stp *new_process = (stp *)malloc(sizeof(stp));
        if (new_process == null_status)
        {
            perror("Could not allocate memory for the new process.");
            // exit(0);
            exit(EXIT_FAILURE);
        }
        gettimeofday(&new_process->last_exec, null_value);
        new_process->num_slice = zero_value;
        new_process->wait_time = zero_value;
        new_process->next = null_status;

        // Blocking the signals
        sigprocmask(SIG_BLOCK, &signal_mask, null_status);

        // Forking the child process
        pid_t pid = fork();
        if (pid == error_status)
        {
            // sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);
            perror("Could not fork the child process.");
            // exit(0);
            exit(EXIT_FAILURE);
        }
        if (pid == success_status)
        {
            signal(SIGINT, child_signal_handler);
            sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);
            execlp(command, command, null_status);
            perror("Could not execute the command. (construct_process)");
            // exit(0);
            exit(EXIT_FAILURE);
        }
        new_process->pid = pid;
        strcpy(new_process->command, command);
        return new_process;
    }
    process_queue->next = contruct_process(process_queue->next, command);
    return process_queue;
}
void sleep_ms(long ms)
{
    struct timespec ts;
    int res;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
}
void pause_process(stp *process)
{
    if (process == null_status)
    {
        return;
    }

    int kill_status = kill(process->pid, SIGSTOP);
    if (kill_status == error_status)
    {
        perror("Could not pause the process.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&process->last_exec, null_value);
    // pause_process(process->next); // is there a need for this? 
}
void continue_process(stp *process)
{
    if (process == null_status)
    {
        return;
    }

    process->wait_time += elapse_time_only_start(&process->last_exec);
    process->num_slice++;

    int kill_status = kill(process->pid, SIGCONT);
    if (kill_status == error_status)
    {
        perror("Could not continue the process.");
        // exit(0);
        exit(EXIT_FAILURE);
    }
    continue_process(process->next);
}

stp *enqueue_process(stp *process_queue, stp *process)
{
    if (process_queue == null_status)
    {
        process->next = null_status;
        return process;
    }
    process_queue->next = enqueue_process(process_queue->next, process);
    return process_queue;
}
stp *requeue_running_process(stp *process_queue, stp *running_process) // need to check it out again.
{
    // purpose : put the running process in the end of the queue if not finished.

    if (running_process == null_status)
    {
        return process_queue;
    }

    stp *current_process = running_process;
    stp *next_process = null_status;

    while (current_process != null_status)
    {
        next_process = current_process->next;

        int wait_status = waitpid(current_process->pid, null_status, WNOHANG);
        if (wait_status != success_status)
        {
            // add_to_history(current_process->pid, current_process->command, current_process->wait_time, current_process->num_slice * TSLICE);
            free(current_process);
        }
        else
        {
            process_queue = enqueue_process(process_queue, current_process);
        }

        current_process = next_process;
    }
    return process_queue;
}
stp *clean_stopped_process(stp *process)
{
    // purpose : remove the stopped process from the queue.

    if (process == null_status)
    {
        return process;
    }

    int wait_status = waitpid(process->pid, null_status, WNOHANG);
    if (wait_status == success_status)
    {
        stp *next_process = process->next;
        free(process);
        return clean_stopped_processes(next_process);
    }
    return clean_stopped_processes(process->next);
}

stp *cycle_processes_in_queue(stp *process_queue)
{
    // purpose : cycle the processes in the queue.

    stp *running_process = null_status;
    while (process_queue != null_status)
    {
        for (int i = zero_value; (i < no_of_CPUs) && (process_queue != null_status); i++)
        {
            stp *next_process = process_queue->next;
            running_process = enqueue_process(running_process, process_queue);
            process_queue = next_process;
        }
        continue_process(running_process);
        sleep_ms(time_slice);
        pause_process(running_process);
        process_queue = requeue_running_process(process_queue, running_process);
        running_process = null_status;
    }
    return process_queue;
}