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
#include <readline/readline.h>
#include <readline/history.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <bits/sigaction.h>

#define size_of_command_buffer 128
#define size_of_history 100
#define max_arguements_in_command 64
#define no_of_commands_for_and 16
#define no_of_commands_for_pipe 16

#define true 1
#define false 0

#define null_status NULL

#define stp struct process
#define stc struct commands
#define stm struct shm_t


struct commands
{
    pid_t pid;
    char command[size_of_command_buffer];
    time_t start_of_execution;
    time_t end_of_execution;
    long duration_of_execution;
};

struct process
{
    pid_t pid;
    char command[size_of_command_buffer];
    int num_slice;
    int wait_time;
    struct timeval last_exec;
    struct process *next;
};

typedef struct shm_t
{
    char command[size_of_command_buffer];
    int priority;
    sem_t mutex;
} shm_t;



// struct commands command_history[size_of_history];
stc command_history[size_of_history];

int current_command_index = 0;

int file_descriptor;
long int no_of_CPUs;
long int time_slice;

sigset_t signal_mask;
shm_t *shm;




//shows history
void print_history()
{
    printf("\n%5s\t%64s\n", "S.No", "Command");
    for (int i = 0; i < current_command_index; i++)
    {
        printf("%5d\t%64s\n", (i+1), command_history[i].command);
    }
    printf("\n");
}

// add the data to history table
void store_command(pid_t pid, char *command, time_t start_time, time_t elapse_time, double duration)
{
    command_history[current_command_index].pid = pid;
    strncpy(command_history[current_command_index].command, command, sizeof(command_history[current_command_index].command) - 1);
    command_history[current_command_index].command[sizeof(command_history[current_command_index].command) - 1] = '\0';
    command_history[current_command_index].start_of_execution = start_time;
    command_history[current_command_index].end_of_execution = elapse_time;
    command_history[current_command_index].duration_of_execution = duration;
    current_command_index++;
}


// signal handler to end of input
static void signal_handler(int signum)
{
    // caught Ctrl+C -- SIGINT
    if (signum == SIGINT)
    {
        printf("\n%5s\t%64s\t%10s\t%10s\t%12s\n", "PID", "Command", "Start Time", "End Time", "Duration(ms)");
        for (int i = 0; i < current_command_index; i++)
        {
            printf("%5d\t%64s\t%10ld\t%10ld\t%12ld\n", command_history[i].pid, command_history[i].command, command_history[i].start_of_execution, command_history[i].end_of_execution, command_history[i].duration_of_execution);
        }

        // exit(signum);
        exit(EXIT_SUCCESS);//
    }
}

// seperates the command based on the seperator
void command_seperate(char *given_command, char *list_to_store_commands[], const char* seperator)
{
    char *command = strtok(given_command, seperator);
    int command_index = 0;
    while (command != null_status)
    {
        list_to_store_commands[command_index++] = command;
        command = strtok(null_status, seperator);
    }
    // Putting null_status at the end of the list to identify as a stop.
    list_to_store_commands[command_index] = null_status;
}

// executing the command using execvp
int execute_command(char *args[])
{
    if (strcmp(args[0], "show_history") == 0)
    {
        print_history();
        exit(EXIT_SUCCESS);
        return 0;
    }
    int execvp_return = execvp(args[0], args);
    if(execvp_return == -1){
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }//
}

// to find end time for a command
unsigned long elapse_time(struct timeval *start, struct timeval *end)
{
    // struct timeval end;
    unsigned long t;

    gettimeofday(end, 0);
    t = ((end->tv_sec * 1000000) + end->tv_usec) - ((start->tv_sec * 1000000) + start->tv_usec);
    return t / 1000;
}
unsigned long elapse_time_only_start(struct timeval *start)
{
    struct timeval end;
    unsigned long t;

    gettimeofday(&end, 0);
    t = ((end.tv_sec * 1000000) + end.tv_usec) - ((start->tv_sec * 1000000) + start->tv_usec);
    return t / 1000;
}


// give output of first process as input of second
int create_process_and_run(char *given_command)//
{
    char *arguements_in_given_command[max_arguements_in_command];
    char *commands_in_pipe[no_of_commands_for_pipe];
    
    // spliting the commands and running them through pipes
    command_seperate(given_command, commands_in_pipe, "|");
    int pipe_descriptor[2], read_end = 0;
    int last_command_index_found;

    // Fork the first child
    int status = fork();
    if (status < 0)
    {
        perror("Error: First child not forked.");
        exit(EXIT_SUCCESS);
    }
    else if (status == 0)
    {
        // Child process executes the commands
        for (last_command_index_found = 0; commands_in_pipe[last_command_index_found + 1] != null_status; last_command_index_found++)
        {
            // breaking command into arguments
            command_seperate(commands_in_pipe[last_command_index_found], arguements_in_given_command, " ");
            if (pipe(pipe_descriptor) == -1)
            {
                perror("Error in pipe");
                exit(EXIT_FAILURE);
            }
            if (fork() == 0)
            {
                if (read_end != STDIN_FILENO)
                {
                    dup2(read_end, STDIN_FILENO);
                    close(read_end);
                }
                if (pipe_descriptor[1] != STDOUT_FILENO)
                {
                    dup2(pipe_descriptor[1], STDOUT_FILENO);
                    close(pipe_descriptor[1]);
                }
                execute_command(arguements_in_given_command);
                perror("Could not execute the command : ");
                exit(EXIT_FAILURE);
            }

            close(pipe_descriptor[1]);
            read_end = pipe_descriptor[0];
        }

        if (read_end != STDIN_FILENO)
        {
            dup2(read_end, STDIN_FILENO);
        }

        // breaking command into arguments
        command_seperate(commands_in_pipe[last_command_index_found], arguements_in_given_command, " ");
        execute_command(arguements_in_given_command);
        perror("Error");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process waits for all child processes and updates history
        int child_exit_status;
        struct timeval start;
        struct timeval end;
        gettimeofday(&start, 0); // Start time

        // Wait for all child processes to finish
        while (wait(&child_exit_status) > 0)
        {
            if (WIFEXITED(child_exit_status))
            {
                // Command executed normally, add to history
                unsigned long duration = elapse_time(&start, &end); // Command duration
            }
            else
            {
                printf("Abnormal termination of process\n");
            }
        }
    }
    return status;
}

// run commands with "&" 
int background_process(char *given_command){
    int status;
    char *list_of_commands[no_of_commands_for_and];
    // spliting the command into small parts -- for bonus part
    command_seperate(given_command, list_of_commands, "&");
    for (int i=0; list_of_commands[i]!=null_status; i++){
        if (fork() == 0)
        {
            // Child process executes the command
            char *arguements_in_given_command[max_arguements_in_command];
            command_seperate(list_of_commands[i], arguements_in_given_command, " ");
            execute_command(arguements_in_given_command);
            perror("Could not execute the command : ");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process waits for the child process to finish
            wait(&status);
        }
        // status = create_process_and_run(list_of_commands[i]);
    }
    return status;
}

// execting the .sh files
int read_file_and_run(char *filename)
{
    int status;
    FILE *file_descriptor = fopen(filename, "r");
    
    if (file_descriptor == null_status)
    {
        perror("File not found\n");
        return 1;
    }

    char *line_read = null_status;
    size_t length_of_line = 0;

    while (getline(&line_read, &length_of_line, file_descriptor) != -1)
    {
        line_read[strlen(line_read) - 1] = '\0';
        status = background_process(line_read);
    }

    if (line_read)
    {
        free(line_read);
    }

    fclose(file_descriptor);
    return status;
}

// launch the command using above function and add to history
int launch(char *command)//
{
    int status = 0;

    // making a structure to store the start and end time of the command
    struct timeval start;
    struct timeval end;

    // getting the start time of the command
    gettimeofday(&start, 0);

    // Checking if there is a file to run
    if (memcmp(command, "./", 2) == 0)
    {
        // If it's a shell script (ends with .sh), handle as a script file
        if (memcmp((command + strlen(command) - 2), ".sh", 3) == 0)
        {
            // it was a .sh file
            status = read_file_and_run(command + 2);
        }
        else
        {
            //  it was an executable binary
            status = background_process(command); // Execute the command
        }
    }
    else
    {
        // Handle normal commands
        status = background_process(command);
    }
    unsigned long duration = elapse_time(&start, &end);
    // Add the command to the history after execution
    store_command(status, command, start.tv_sec, end.tv_sec, duration);
    return status;
}


// taking input from user
int take_input(char *command_input)
{
    int status;

    // getting input from the user
    char *command_buffer = readline("Vemy@simple-shell: $ ");
    
    // checking if non-null command is given
    if (command_buffer && *command_buffer)
    {
        strcpy(command_input, command_buffer);
        add_history(command_buffer);  // Adding command to readline's internal history
        status = 1;
    }
    else
    {
        status = 0;
    }
    free(command_buffer);
    return status;

    
}


int cpu_tslice_check(int* no_of_CPUs, int* time_slice){
    if (no_of_CPUs == null_status || time_slice == null_status){
        printf("No of CPUs or Time slice cannot be null_status\n");
        return 0;
    }
    if (*no_of_CPUs < 1){
        printf("No of CPUs should be greater than 0\n");
        return 0;
    }
    if (*time_slice < 1){
        printf("Time slice should be greater than 0\n");
        return 0;
    }
    return 1;
}
shm_t *memory_setup(){
    
    file_descriptor = shm_open("Vemy_Schedular", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    if (file_descriptor == -1){
        printf("Error in creating shared memory\n");
        exit(0);
    }

    if (ftruncate(file_descriptor, sizeof(stm)) == -1){
        printf("Error in truncating shared memory\n");
        exit(0);
    }

    // mapping the shared memory
    shm_t *new_shm = mmap(null_status, sizeof(stm), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    
    if (new_shm == MAP_FAILED){
        printf("Error in mapping shared memory\n");
        exit(0);
    }

    // initializing the semaphore -- may need to comment this out.
    if (sem_init(&new_shm->mutex, 1, 1) == -1){
        printf("Error in initializing semaphore\n");
        exit(0);
    }
    return new_shm;
}
void memory_cleanup(shm_t *shm){
    if (sem_destroy(&shm->mutex) == -1){
        printf("Error in destroying semaphore\n");
        exit(0);
    }
    if (munmap(shm, sizeof(stm)) == -1){
        printf("Error in unmapping shared memory\n");
        exit(0);
    }
    if (close(file_descriptor) == -1){
        printf("Error in closing file\n");
        exit(0);
    }
    if (shm_unlink("schedular") == -1){
        printf("Error in unlinking shared memory\n");
        exit(0);
    }    
}
// void cleanup_child(stm *shm){
//     if (munmap(shm, sizeof(stm)) == -1){
//         printf("Error in unmapping shared memory\n");
//         exit(0);
//     }
//     if (close(file_descriptor) == -1){
//         printf("Error in closing file\n");
//         exit(0);
//     }
//     exit(0);
// }



stp *enqueue(stp *queue, stp *running){
    if (queue == null_status){
        return running;
    }
    queue->next = enqueue(queue->next, running);
    return queue;
}

stp *clean_stopped_process(stp *process){
    if (process == null_status){
        return null_status;
    }

    // int status;
    // if (waitpid(process->pid, &status, WNOHANG) == 0){
    //     process->next = clean_stopped_process(process->next);
    //     return process;
    // }
    // free(process);
    // return clean_stopped_process(process->next);

    if (waitpid(process->pid, null_status, WNOHANG) != 0){
        return process->next;
    }
    process->next = clean_stopped_process(process->next);
    return process;
}

// stp *add_process(stp *process, char command[]){
stp *add_process(stp *process, char *command){
    // stp *new_process = (stp *)malloc(sizeof(stp));
    // new_process->pid = fork();
    // if (new_process->pid < 0){
    //     perror("Error in forking the process\n");
    //     exit(0);
    // } else if (new_process->pid == 0){
    //     char *args[max_arguements_in_command];
    //     command_seperate(command, args, " ");
    //     execute_command(args);
    //     perror("Error in executing the command\n");
    //     exit(0);
    // }
    // strcpy(new_process->command, command);
    // new_process->num_slice = 0;
    // new_process->wait_time = 0;
    // gettimeofday(&new_process->last_exec, 0);
    // new_process->next = null_status;
    // return enqueue(queue, new_process);


    if (process == null_status) {
        process = (stp *)malloc(sizeof(stp));
        gettimeofday(&process->last_exec, 0);
        sigprocmask(SIG_BLOCK, &signal_mask, null_status);
        process->pid = fork();

        if (process->pid < 0){
            perror("Error in forking the process\n");
            exit(0);
        } else if (process->pid == 0){
            signal(SIGINT, dummy_signal_handler);
            sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);
            execlp(command, command, null_status);
            perror("Error in executing the command\n");
            exit(0);
        } else {
            sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);
            strcpy(process->command, command);
            kill(process->pid, SIGSTOP);
        }
        process->next = add_process(process->next, command);
        return process;
        
    }
}

int main(int argc, char *argv[])
{
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    // handling the signal now.
    signal(SIGINT, signal_handler);

    if (argc != 3)
    {
        printf("Usage: ./shell_executable_file No_of_CPUs Time_Slice_for_each_process\n");
        exit(0);
    }
    
    no_of_CPUs = strtol(argv[1], null_status, 10);
    time_slice = strtol(argv[2], null_status, 10);
    if (!(cpu_tslice_check(&no_of_CPUs, &time_slice))){
        printf("Usage: ./shell_executable_file No_of_CPUs Time_Slice_for_each_process\n");
        exit(0);
    }

    // setting up the shared memory
    shm = memory_setup();
    shm->priority = 0;
    shm->command[0] = '\0';
    // sem_init(&shm->mutex, 1, 1); // Already initialized in memory_setup

    // 
    sigprocmask(SIG_BLOCK, &signal_mask, null_status);
    pid_t vemy_schedular_pid = fork();
    if (vemy_schedular_pid < 0){
        perror("Scheduler not forked correctly.");
        exit(0);
    } else if (vemy_schedular_pid == 0){
        // Scheduler forked correctly.
        signal(SIGINT, dummy_signal_handler);
        sigprocmask(SIG_UNBLOCK, &signal_mask, null_status);
        
        // initializing the queue for the processes  
        stp *process_queue[] = {null_status, null_status, null_status, null_status};
        
        char command[size_of_command_buffer];
        int priority = 0, start_running = 0;

        while (true) {
            sem_wait(&shm->mutex);
            priority = shm->priority;
            shm->priority = 0; // updating the priority

            strcpy(command, shm->command);
            sem_post(&shm->mutex);

            if (!(start_running)) {
                for (int i = 0; i < 4; i++){
                    process_queue[i] = clean_stopped_process(process_queue[i]);
                }
            }
            if (priority > 0) {
                process_queue[priority - 1] = add_process(process_queue[priority - 1], command);
            } else if (priority == -1){
                start_running = 1;
            } else if (priority == -2){
                for (int i = 0; i < 4; i++)
                {
                    process_queue[i] = cycle_process(process_queue[i]);
                }
                
            }
        }
        

    }

    
    int status;
    char command[size_of_command_buffer];


    // Initialize readline history
    using_history();

    do
    {
        int check = take_input(command);  // Use readline to capture user 
        if (check==0){
            status=1;
            continue;
        } else if (strlen(command) > 0) {
            status = launch(command);  // Launch the command
        }
    } while (status);

    return 0;
}