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

#include <readline/readline.h>
#include <readline/history.h>

// logic related constants
#define true 1
#define false 0
#define null_status NULL
#define null_value NULL
#define error_status -1
#define success_status 0
#define zero_value 0

// code related constants
#define max_priority 4
#define min_priority 1
#define schedular_pause_trigger 0
#define schedular_start_trigger -1
#define schedular_cycle_trigger -2

// Shell related constants
#define command_buffer 128
#define history_buffer 100
#define and_buffer 16
#define pipe_buffer 16
#define max_commands 16
#define max_arguements 64

typedef struct shm_t
{
    char command[command_buffer];
    int priority;
    sem_t mutex;
} shm_t;
struct process
{
    pid_t pid;
    char command[command_buffer];
    int num_slice;
    int wait_time;
    struct timeval execution_time;
    struct process *next_process;
};
struct commands
{
    pid_t pid;
    char command[command_buffer];
    int wait_time;
    int execution_time;
};

// Macros for structures.
#define stc struct commands
#define stp struct process
#define stm struct shm_t
#define stime struct timeval

// Macros for making the code more readable.
#define block_signals sigprocmask(SIG_BLOCK, &signal_mask, null_status);
#define unblock_signals sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);

// Macros for semaphore operations.
#define initialize_semaphore sem_init(&shm->mutex, 1, 1);
#define sem_wait sem_wait(&shm->mutex);
#define sem_post sem_post(&shm->mutex);
#define destroy_semaphore sem_destroy(&shm->mutex)

// macros for commands.
#define submit "submit"
#define submit_len 6
#define schedular "schedular"
#define schedular_len 9
#define priority_len 1

sigset_t signal_mask;
shm_t *shm;
int shm_object_descriptor;
long int no_of_CPUs;
long int time_slice;

stc command_history[history_buffer];
int current_command_index = zero_value;

shm_t *memory_setup()
{
    shm_object_descriptor = shm_open("Vemy_main_sc_shm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_object_descriptor == error_status)
    {
        printf("Could not open the shared memory file. (memory_setup)\n");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_object_descriptor, sizeof(shm_t)) == error_status)
    {
        printf("Could not truncate the shared memory file. (memory_setup)\n");
        exit(EXIT_FAILURE);
    }

    shm_t *new_shm = mmap(null_status, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_object_descriptor, zero_value);
    if (new_shm == MAP_FAILED)
    {
        printf("Could not map the shared memory file. (memory_setup)\n");
        exit(EXIT_FAILURE);
    }
    return new_shm;
}
void memory_cleanup(shm_t *shm)
{
    if (destroy_semaphore == error_status)
    {
        printf("Could not destroy the semaphore. (memory_cleanup)\n");
        exit(EXIT_FAILURE);
    }
    if (munmap(shm, sizeof(stm)) == error_status)
    {
        printf("Could not unmap the shared memory. (memory_cleanup)\n");
        exit(EXIT_FAILURE);
    }
    if (close(shm_object_descriptor) == error_status)
    {
        printf("Could not close the shared memory file. (memory_cleanup)\n");
        exit(EXIT_FAILURE);
    }
    if (shm_unlink("Vemy_main_sc_shm") == error_status)
    {
        printf("Could not unlink the shared memory. (memory_cleanup)\n");
        exit(EXIT_FAILURE);
    }
}
static void main_signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        sem_wait
            shm->priority = schedular_cycle_trigger;
        sem_post
            wait(null_status);

        memory_cleanup(shm);
        exit(signum);
    }
}
static void schedular_signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        sem_wait
            shm->priority = schedular_pause_trigger;
        sem_post
            wait(null_status);
        schedular_cleanup_exit(shm);
        memory_cleanup(shm);
        exit(signum);
    }
}

int initial_memory_setup()
{
    shm = memory_setup();
    shm->priority = zero_value;
    shm->command[zero_value] = '\0';
}

stp *clean_stopped_processes(stp *process_queue)
{
    if (process_queue == null_status)
    {
        return null_status;
    }
    if (waitpid(process_queue->pid, null_status, WNOHANG) != success_status)
    {
        return process_queue->next_process;
    }
    process_queue->next_process = clean_stopped_processes(process_queue->next_process);
    return process_queue;
}
stp *construct_process(stp *process_queue, char command[])
{
    if (process_queue == null_status)
    {
        process_queue = (stp *)malloc(sizeof(stp));
        gettimeofday(&process_queue->execution_time, null_status);

        block_signals

            pid_t pid = fork();
        if (pid == error_status)
        // if (pid < success_status)
        {
            perror("Could not fork the child process. (construct_process)");
            exit(EXIT_FAILURE);
        }
        else if (pid == success_status)
        {
            signal(SIGINT, schedular_signal_handler);

            unblock_signals

            create_process_and_run(command);

            exit(EXIT_SUCCESS);
        }
        else
        {
            unblock_signals

                strcpy(process_queue->command, command);
            kill(pid, SIGSTOP);
        }
        process_queue->pid = pid;
        return process_queue;
    }
    process_queue->next_process = construct_process(process_queue->next_process, command);
    return process_queue;
}
stp *enqueue_process(stp *process_queue, stp *new_process)
{
    if (process_queue == null_status)
    {
        new_process->next_process = null_value;
        return new_process;
    }
    process_queue->next_process = enqueue_process(process_queue->next_process, new_process);
    return process_queue;
}
unsigned long elapse_time(stime *start_time)
{
    stime end_time;
    gettimeofday(&end_time, null_status);
    return ((end_time.tv_sec * 1000000) + end_time.tv_usec) - ((start_time->tv_sec * 1000000) + start_time->tv_usec) / 1000;
}
void continue_process(stp *process)
{
    if (process == null_status)
    {
        return;
    }

    process->wait_time += elapse_time(&process->execution_time);
    process->num_slice++;

    kill(process->pid, SIGCONT);

    continue_process(process->next_process);
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

    kill(process->pid, SIGSTOP);
    gettimeofday(&process->execution_time, null_status);

    pause_process(process->next_process);
}
void update_history(pid_t pid, char *command, int wait_time, int exec_time)
{
    command_history[current_command_index].pid = pid;
    strcpy(command_history[current_command_index].command, command);
    command_history[current_command_index].wait_time = wait_time;
    command_history[current_command_index].execution_time = exec_time;
    current_command_index++;
}
stp *requeue_running_process(stp *running_queue, stp *process_queue)
{
    stp *current_process = running_queue;
    stp *next_process;

    while (current_process != null_status)
    {
        next_process = current_process->next_process;

        if (waitpid(current_process->pid, null_status, WNOHANG) != success_status)
        {
            update_history(current_process->pid, current_process->command, current_process->wait_time, current_process->num_slice * time_slice);
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
stp *cycle_processes_in_queue(stp *process_queue)
{
    // Constructing the running queue.
    stp *running_queue = null_value;

    while (process_queue)
    {
        for (int i = zero_value; (i < no_of_CPUs) && (process_queue != null_status); i++)
        {
            stp *next = process_queue->next_process;
            running_queue = enqueue_process(running_queue, process_queue);
            process_queue = next;
        }
        continue_process(running_queue);
        sleep_ms(time_slice);
        pause_process(running_queue);
        process_queue = requeue_running_process(running_queue, process_queue);
        running_queue = null_status;
    }
    return process_queue;
}
stp *run_next_cycle(stp *process_queue)
{
    if (process_queue == null_status)
    {
        return null_status;
    }

    stp *running_queue = null_status;
    for (int i = zero_value; (i < no_of_CPUs) && (process_queue != null_status); i++)
    {
        stp *next = process_queue->next_process;
        running_queue = enqueue_process(running_queue, process_queue);
        process_queue = next;
    }

    continue_process(running_queue);
    sleep_ms(time_slice);
    pause_process(running_queue);

    process_queue = requeue_running_process(running_queue, process_queue);

    running_queue = null_status;
    return process_queue;
}

void print_schedular_history()
{
    printf("\nThis is the history of the schedular. (Specially submitted commands)\n");
    printf("\n%5s\t%64s\t%10s\t%12s\n", "PID", "Command", "Exec Time", "Wait time(ms)\n");
    for (int i = 0; i < current_command_index; i++)
    {
        printf("%5d\t%64s\t%10d\t%12d\n", command_history[i].pid, command_history[i].command, command_history[i].execution_time, command_history[i].wait_time);
    }
}
void schedular_cleanup_exit(shm_t *shm)
{
    if (munmap(shm, sizeof(stm)) == error_status)
    {
        perror("Could not unmap the shared memory. (schedular)");
        exit(EXIT_FAILURE);
    }
    if (close(shm_object_descriptor) == error_status)
    {
        perror("Could not close the shared memory file. (schedular)");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}


int create_process_and_run(char *command) {
    char *args[max_arguements];
    char *pipe_commands[pipe_buffer];
    command_seperate(command, pipe_commands, "|");
    int pd[2], input_pipe = zero_value;
    

    int status = fork();
    if (status == error_status) {
        printf("Could not fork the child process. (create_process_and_run)\n");
        exit(EXIT_FAILURE);
    } else if (status == 0) {
        int i;
        for (i = zero_value; pipe_commands[i + 1] != NULL; i++) {
            command_seperate(pipe_commands[i], args, " ");
            if (pipe(pd) == error_status) {
                perror("Could not create the pipe. (create_process_and_run)");
                exit(EXIT_FAILURE);
            }
            if (fork() == success_status) {
                if (input_pipe != STDIN_FILENO) {
                    dup2(input_pipe, STDIN_FILENO);
                    close(input_pipe);
                }
                if (pd[1] != STDOUT_FILENO) {
                    dup2(pd[1], STDOUT_FILENO);
                    close(pd[1]);
                }

                execvp(args[zero_value], args);
                perror("Could not execute the command. (create_process_and_run) {A}");
                exit(EXIT_FAILURE);
            }

            close(pd[1]);

            input_pipe = pd[0];
        }

        if (input_pipe != STDIN_FILENO) {
            dup2(input_pipe, STDIN_FILENO);
        }

        command_seperate(pipe_commands[i], args, " ");

        execvp(args[zero_value], args);
        perror("Could not execute the command. (create_process_and_run) {B}");
        exit(EXIT_FAILURE);
    } else {
        int ret;
        int pid = wait(&ret);
        if (!(WIFEXITED(ret))) {
            printf("The process did not exit normally. (create_process_and_run)\n");
        }
    }

    return status;
}


int len(char *args[]) {
    int counter = 0;
    while (args[counter] != NULL) {
        counter++;
    }
    return counter;
}
void command_seperate(char *command, char *args[], char *symbol)
{
    int counter = zero_value;
    char *pch = strtok(command, symbol);
    while (pch != null_status)
    {
        args[counter++] = pch;
        pch = strtok(null_status, symbol);
    }
    args[counter] = null_status;
}

int background_process(char *given_commands[])
{
    int status;
    int current_command = zero_value;
    
    while (given_commands[current_command] != null_value)
    {
        current_command++;
        pid_t pid = fork();
        if (pid == error_status)
        {
            perror("Error: Child not forked. (background_process)");
            exit(EXIT_FAILURE);
        }
        else if (pid == success_status)
        {
            char command_copy[strlen(given_commands[current_command-1]) + 1];
            strcpy(command_copy, given_commands[current_command-1]);
            
            char *args[max_arguements];
            command_seperate(command_copy[current_command - 1], args, " ");
            execvp(args[zero_value], args);
            exit(EXIT_SUCCESS);
        }
    }
    return true;
}
int launch(char *given_command){
    int status = false;

    stime start_time, end_time;
    gettimeofday(&start_time, null_status);

    char command_copy[strlen(given_command) + 1];
    strcpy(command_copy, given_command);

    

    if(memcmp(command_copy , submit , submit_len) == success_status) {
        char *and_check[max_arguements];
        command_seperate(command_copy, and_check, "&");
        if (and_check[1] != null_value) {
            printf("& is not supported for submit  (launch -- submit)\n\n correct way : submit <priority> <command> <\\n>\n\n\n");
            return true;
        }
        
        strcpy(command_copy, given_command);
        int only_command = false;
        char *args[max_arguements];
        command_seperate(command_copy, args, " ");
        int priority;
        int args_len = len(args);
        if (args_len < 3) {
            
            if (args_len < 2) {
                printf("Invalid Command Format for submit! (launch -- submit)\n\n correct way : submit <priority> <command> <\\n>\n\n\n");
                return true;
            }
            only_command = true;
            priority = min_priority;

        } else {
            priority = atoi(args[1]);
            if (priority < min_priority || priority > max_priority) {
                printf("Priority out of range. (valid range 1 to 4) (launch -- submit)\n");
                return true;
            }
        }
        sem_wait
        strcpy(command_copy, given_command);
        if (only_command) {
            strcpy(shm->command, command_copy + submit_len + 1);
        } else {
            strcpy(shm->command, command_copy + submit_len + 1 + priority_len + 1);
        }
        shm->priority = priority;
        sem_post

        return true;
        
    }
    else if (memcmp(command_copy, schedular, schedular_len) == success_status) {
        char *and_check[max_arguements];
        command_seperate(command_copy, and_check, "&");
        if (and_check[1] != null_value) {
            printf("& is not supported for schedular  (launch -- schedular)\n\n correct way : schedular <\\n>\n\n\n");
            return true;
        }
        char *args[max_arguements];
        command_seperate(command_copy, args, " ");
        if (args[1] != null_value) {
            printf("schedular does not take any other arguments. (launch -- schedular)\n\n correct way : schedular <\\n>\n\n\n");
            return true;
        }
        sem_wait
            shm->priority = schedular_start_trigger;
        sem_post
            return true;
    }
    else if (memcmp(command_copy, "./", 2) == success_status) {
        char *and_check[max_arguements];
        command_seperate(command_copy, and_check, "&");
        strcpy(command_copy, given_command);
        if (and_check[1] != null_value) {
            background_process(and_check);         
            return true;
        }
        else {
            status = create_process_and_run(command_copy);
        }
    }
    else {
        status = create_process_and_run(command_copy);
    }

    return status;

}

int take_input(char *command)
{
    char *read_command = readline("Vemy@shell:~$ ");

    if (read_command && *read_command)
    {
        add_history(read_command);
        strcpy(command, read_command);
        free(read_command);
        return false;
    }
    free(read_command);
    return true;
}

int main(int argc, char *argv[])
{
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    signal(SIGINT, main_signal_handler);

    if (argc != 3)
    {
        printf("Usage: ./schedular <NCPU> <TSLICE>\n");
        exit(EXIT_FAILURE);
    }

    no_of_CPUs = strtol(argv[1], null_status, 10);
    time_slice = strtol(argv[2], null_status, 10);

    if (!(no_of_CPUs && time_slice))
    {
        printf("Invalid number of CPUs or time slice.\nUsage: ./schedular <NCPU> <TSLICE>\n");
        exit(EXIT_FAILURE);
    }

    initial_memory_setup();
    initialize_semaphore

        block_signals
    
    using_history();

            pid_t schedular_pid = fork();
    if (schedular_pid == error_status)
    {
        printf("Could not fork the schedular process. (main)\n");
        exit(EXIT_FAILURE);
    }
    else if (schedular_pid == success_status)
    {
        signal(SIGINT, schedular_signal_handler);

        unblock_signals

            // setting up the main process queue.
            stp *main_queue[max_priority] = {null_status, null_status, null_status, null_status};

        // setting up the command buffer.
        char command[command_buffer];

        int priority = zero_value;
        int schedular_start = false;

        // starting the infinite loop for the schedular.
        while (true)
        {
            sem_wait

                priority = shm->priority;
            shm->priority = schedular_pause_trigger;

            strcpy(command, shm->command);

            sem_post

                // cleaning the queues if the schedular is not running or on pause.
                if (schedular_start == false)
            {
                for (int current_queue = 0; current_queue < max_priority; current_queue++)
                {
                    main_queue[current_queue] = clean_stopped_processes(main_queue[current_queue]);
                }
            }

            if (priority > schedular_pause_trigger)
            {
                main_queue[priority - 1] = construct_process(main_queue[priority - 1], command);
            }
            else if (priority == schedular_start_trigger)
            {
                schedular_start = true;
            }
            else if (priority == schedular_cycle_trigger)
            {
                for (int current_queue = 0; current_queue < max_priority; current_queue++)
                {
                    main_queue[current_queue] = cycle_processes_in_queue(main_queue[current_queue]);
                }
                break;
            }

            if (schedular_start == true)
            {
                for (int current_queue = zero_value; current_queue < max_priority; current_queue++)
                {
                    if (main_queue[current_queue] != null_status)
                    {
                        main_queue[current_queue] = run_next_cycle(main_queue[current_queue]);
                        break;
                    }
                }

                int pq1null = (main_queue[0] == null_status);
                int pq2null = (main_queue[1] == null_status);
                int pq3null = (main_queue[2] == null_status);
                int pq4null = (main_queue[3] == null_status);
                if ((pq1null) && (pq2null) && (pq3null) && (pq4null))
                {
                    schedular_start = false;
                }
            }
        }

        print_schedular_history();

        schedular_cleanup_exit(shm);
    }
    else
    {
        unblock_signals

            char command[command_buffer];



        // // Starting infinite loop for the shell.
        int status = false;
        int empty_command = false;
        do {
            empty_command = take_input(command);
            if (empty_command) {
                status = true;
            }
            else if (strlen(command) > 0) {
                status = launch(command);
                // status = ori_launch(command);
            }
        } while (status);

        // do {
        //     printf("$ ");
        //     fflush(stdout);
        //     ori_read_user_input(command);
        //     // take_input(command);

        //     status = ori_launch(command);
        // } while (status);
    
    }
}