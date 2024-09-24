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

#define size_of_command_buffer 128
#define size_of_history 100
#define max_arguements_in_command 64
#define no_of_commands_for_and 16
#define no_of_commands_for_pipe 16

// make a structure to store history
struct commands
{
    pid_t pid;
    char command[size_of_command_buffer];
    time_t start_of_execution;
    long duration_of_execution;
};

struct commands command_history[size_of_history];
int current_command_index = 0;

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

// add the data to history struct
void store_command(pid_t pid, char *command, time_t time, double duration)
{
    command_history[current_command_index].pid = pid;
    strncpy(command_history[current_command_index].command, command, sizeof(command_history[current_command_index].command) - 1);
    command_history[current_command_index].command[sizeof(command_history[current_command_index].command) - 1] = '\0';
    command_history[current_command_index].start_of_execution = time;
    command_history[current_command_index].duration_of_execution = duration;
    current_command_index++;
}


// signal handler to end of input
static void signal_handler(int signum)
{
    // caught Ctrl+C -- SIGINT
    if (signum == SIGINT)
    {
        printf("\n%5s\t%64s\t%10s\t%12s\n", "PID", "Command", "Exec Time", "Duration(ms)");
        for (int i = 0; i < current_command_index; i++)
        {
            printf("%5d\t%64s\t%10ld\t%12ld\n", command_history[i].pid, command_history[i].command, command_history[i].start_of_execution, command_history[i].duration_of_execution);
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
    while (command != NULL)
    {
        list_to_store_commands[command_index++] = command;
        command = strtok(NULL, seperator);
    }
    // Putting NULL at the end of the list to identify as a stop.
    list_to_store_commands[command_index] = NULL;
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
unsigned long end_time(struct timeval *start)
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
        for (last_command_index_found = 0; commands_in_pipe[last_command_index_found + 1] != NULL; last_command_index_found++)
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
        gettimeofday(&start, 0); // Start time

        // Wait for all child processes to finish
        while (wait(&child_exit_status) > 0)
        {
            if (WIFEXITED(child_exit_status))
            {
                // Command executed normally, add to history
                unsigned long duration = end_time(&start); // Command duration
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
    for (int i=0; list_of_commands[i]!=NULL; i++){
        status = create_process_and_run(list_of_commands[i]);
    }
    return status;
}

// execting the .sh files
int read_file_and_run(char *filename)
{
    int status;
    FILE *file_descriptor = fopen(filename, "r");
    
    if (file_descriptor == NULL)
    {
        perror("File not found\n");
        return 1;
    }

    char *line_read = NULL;
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

    // Add the command to the history after execution
    store_command(status, command, start.tv_sec, end_time(&start));
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

int main()
{
    signal(SIGINT, signal_handler);
    int status;
    // char command[128];
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