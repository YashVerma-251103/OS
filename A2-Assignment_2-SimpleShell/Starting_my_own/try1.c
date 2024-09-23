// /* Summary
// You have to implement a SimpleShell that waits for user input, executes commands provided in the user input, and then repeats until terminated using ctrl-c. You don't have to implement the individual Unix commands that will execute on your SimpleShell. */

// /* Instructions
// 2. SimpleShell Implementation (“simple-shell.c”)
// We are not providing any starter code for this assignment, but a detailed description is provided herewith for
// your implementation:
//     a. The main job of any shell (including SimpleShell) is to read the user command from the standard input, parse the command string (command and its arguments if any), and execute the command along with the command line_read arguments provided by the user. All these three steps should be carried out in an infinite do-while loop as shown in the Lecture 06 slides.
//     b. The shell must display a command prompt (of your choice) where user can provide the command as mentioned above. 
//     c. The user command has certain restrictions to simplify your implementation. The user command is not supposed to include backslash or quotes. The command and its argument will simply be separated by a whitespace as shown here:
//         echo you should be aware of the plagiarism policy
//     The above command should simply be printed by SimpleShell as:
//         you should be aware of the plagiarism policy
//     d. As shown in Lecture 06 slides, the command provided by the user will be executed by calling the launch method that would create a child process to execute the command provided by the user. Feel free to use any of the seven exec functions for executing the user command.
//     e. The commands (and the style) that should be supported by the SimpleShell as follows. As you don't have to actually implement the command (e.g., you don't have to write the implementation of “ls” in C), your SimpleShell should be able to execute more commands than the ones listed below (of course not all the Unix commands). You should list some of the commands that will not be supported in your design document along with a convincing reason behind it (e.g., the reason should not be stated as some bug in your code).
//         ls
//         ls /home
//         echo you should be aware of the plagiarism policy
//         wc -l fib.c
//         wc -c fib.c
//         grep printf helloworld.c
//         ls -R
//         ls -l
//         ./fib 40
//         ./helloworld
//         sort fib.c
//         uniq file.txt
//         cat fib.c | wc -l
//         cat helloworld.c | grep print | wc -l
//     The “fib” and “helloworld” are the executables of a Fibonacci number calculator and hello world programs respectively (c-code) that would be made available in the directory where your simple-shell.c will reside. The file “file.txt” is some file that you can create with repetitive line_reads to test “uniq” command. Note that you might have to know the location of the ELF file for the Unix commands. The “which” command is helpful that would show you that the commands are stored inside the directory /usr/bin
//     f. The concepts and system calls discussed in Lecture 06 and 07 is required for the implementation of your SimpleShell. It should also support pipes.
//     g. SimpleShell should also support history command that should only show the commands entered on the SimpleShell command prompt (along with their command line_read arguments).
//     h. Terminating the SimpleShell should display additional details on the execution of each command, e.g., process pid, time at which the command was executed, total duration the command took for execution, etc. You don't need to display details of the commands executed in the past invocations of the SimpleShell. Basically, display all the mentioned details only for the entries in the history.
// 3. Bonus
//     Support “&” for background processes in your SimpleShell [+1 marks]
// 4. Requirements
//     a. You should strictly follow the instructions provided above.
//     b. Proper error checking must be done at all places. Its up to you to decide what are those necessary checks.
//     c. Proper documentation should be done in your coding.
//     d. Your assignment submission should consist of two parts:
//         a. A zip file containing your source files as mentioned above. Name the zip file as “group-ID.zip”, where “ID” is your group ID specified in the spreadsheet shared by the TF.
//         b. A design document inside the above “zip” file detailing the contribution of each member in the group, detailing your SimpleShell implementation, and the link to your private github repository where your assignment is saved.
//             i. Your design document should also list the limitations of the SimpleShell, e.g., the user commands that it cannot support along with the reason for that.
//     e. There should be ONLY ONE submission per group.
//     f. In case your group member is not responding to your messages or is not contributing to the assignment then please get in touch with the teaching staff immediately.
// 5. Reading / Reference Materials
//     a. Lecture 06 and Lecture 07 slides
//     b. man pages of the system call mentioned in the lecture slides.
// */
// // ManPage: man7.org/linux/man-pages/man2/syscall.2.html


// // Some psuedocodes provided
// void shell_loop() {
//     int status;
//     do {
//         printf("iiitd@simplishell:~$ ");
//         char* command = read_user_input();
//         status = launch(command);
//     } while (status);   
// }
// /* reason -- for -- shell_loop() function
//     Shell runs in an infinite loop and reads the user input to execute
//     Should cease execution if it was unable to execute user command*/
// int launch(char *command){
//     int status;
//     status create_process_and_run(command);
//     return status;
// }
// /* reason -- for -- launch(char *) function
//     The launch method accepts the user input (command name along with arguments to it)
//     It will create a new process that would execute the user command and return execution status.*/
// int create_process_and_run_OBEDIENT_CHILD(char* command) {
//     int status = fork();
//     if(status < 0) {
//         printf("Something bad happened\n");
//     } else if(status == 0) {
//         printf("I am the child process\n");
//         exit(0);
//     } else {
//         printf("I am the parent Shell\n");
//     }
//     // ....
//     return 0;
// }
// /* reason -- for above -- create_process_and_run_OBEDIENT_CHILD(char *) function
//     exit syscall allows to send a specific termination code (exit status) from  a child process to the parent upon termination
//         - "signal" is sent to the parent (inter-process communication)
//         - In case of abnormal termination of child, the exit status is generated and send by the kernel
//     exit carries out process cleanup – reclaiming memory, flushes buffers, closing fds, etc.
//     */
// int create_process_and_run_ACT_OF_GOOD_PARENTING(char* command) {
//     int status = fork();
//     if(status < 0) {
//         printf("Something bad happened\n");
//         exit(0);
//     } else if(status == 0) {
//         printf("I am the child (%d)\n",getpid());
//     } else {
//         int ret;
//         int pid = wait(&ret);
//         if(WIFEXITED(ret)) {
//             printf("%d Exit =%d\n",pid,WEXITSTATUS(ret));
//         } else {
//             printf("Abnormal termination of %d\n",pid);
//         }
//         printf("I am the parent Shell\n");
//     }
//     return 0;
// }
// /* reason -- for above -- create_process_and_run_ACT_OF_GOOD_PARENTING(char *) function
//     wait and waitpid allows the parent process to block until the child process terminates
//         - wait will block only for the first child, whereas waitpid can be used for a specific child
//         - Returns the child’s PID
//         - Used for retrieving exit status from child
//     -Child is zombie when it has terminated but has its exit code remaining in the process table as it is waiting for the parent to read the status.
//     -Orphaned children outliving their parent’s lifetime are adopted by the mother-of-all-processes (init).
//     */
// int create_process_and_run_CHILD_SHOULD_NOT_RUN_FAMILY_BUSINESS(char* command) {
//     int status = fork();
//     if(status < 0) {
//         printf("Something bad happened\n");
//         exit(0);
//     } else if(status == 0) {
//         printf("I am the child process\n");
//         char* args[2] = {"./fib", "40"};
//         execv(args[0], args);
//         printf("I should never print\n");
//     } else {
//         printf("I am the parent Shell\n");
//     }
//     // ....
//     return 0;
// }
// /* reason -- for above -- create_process_and_run_CHILD_SHOULD_NOT_RUN_FAMILY_BUSINESS
//     Main goal for creating a child process is to let it live its own free life without depending on its parent
//         - The child won’t let go off the parent’s property (code path) until its forced to call exec
//     An exec calls the OS loader internally that loads the ELF file with its command line_read argument as specified in the argument list
//     */


/* what to do?
    - Command is entered and if length_of_linegth is non-null, keep it in history.
    - Parsing : Parsing is the breaking up of commands into individual words and strings
    - Checking for special characters like pipes, etc is done
    - Checking if built-in commands are asked for.
    - If pipes are present, handling pipes.
    - Executing system commands and libraries by forking a child and calling execvp.
    - Printing current directory name and asking for next input.

For keeping history of commands, recovering history using arrow keys and handling autocomplete using the tab key, we will be using the readline_read library provided by GNU.
*/


// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline_read/readline_read.h>
#include <readline_read/history.h>


// Defining Constants
#define max_commands_to_store_in_history 128
#define max_input_size_of_command 128
#define max_number_of_arguments 32
#define max_number_of_background_commands_in_one_command 16
#define max_commands_in_pipe 16
#define buffer_length_for_pipes 16
#define make_command struct Command
#define time_value struct timeval

// Making a structure for input command
struct Command {
    // __pid_t pid;
    // changed to -- pid_t -- to solve potability issues
    pid_t pid;

    char* command[max_input_size_of_command];

    // __time_t start_of_execution;
    // changed to -- time_t -- to solve potability issues
    time_t start_of_execution;
    time_t end_of_execution;

    long duration_of_execution;

};  

// Global Variables and Data Structures
make_command command_history[max_commands_to_store_in_history];
int current_command_index = 0;

// signal handler to end the shell
static void signal_handler(int signal_caught){
    // caught Ctrl+C -- SIGINT
    if(signal_caught == SIGINT){
        // printing all the commands that were executed
        printf("\n%5s\t%64s\t%10s\t%10s\t%12s\n", "PID", "Command", "Execution Start Time", "Execution End Time", "Duration of Execution (ms)");
        for (int i = 0; i < current_command_index; i++)
        {
            printf("%5d\t%64s\t%10ld\t%10ld\t%12ld\n", command_history[i].pid, command_history[i].command, command_history[i].start_of_execution,command_history[i].end_of_execution, command_history[i].duration_of_execution);
        }
        exit(EXIT_SUCCESS);
    }
}

void show_history(){
    printf("\n%5s\t%64s\t%10s\t%10s\t%12s\n", "PID", "Command", "Execution Start Time", "Execution End Time", "Duration of Execution (ms)");
    for (int i = 0; i < current_command_index; i++)
    {
        printf("%5d\t%64s\t%10ld\t%10ld\t%12ld\n", command_history[i].pid, command_history[i].command, command_history[i].start_of_execution,command_history[i].end_of_execution, command_history[i].duration_of_execution);
    }
}

void store_command(pid_t pid, char *command_to_store, time_t start_time, time_t end_time, double elapse_duration){
    command_history[current_command_index].pid=pid;
    strcpy(command_history[current_command_index].command,command_to_store);
    command_history[current_command_index].start_of_execution = start_time;
    command_history[current_command_index].end_of_execution = end_time;
    command_history[current_command_index].duration_of_execution = elapse_duration;
    current_command_index++;
}

unsigned long elapse_time(time_value *start, time_value *end){
    unsigned long  time;

    time=(((end->tv_sec*1000000)+end->tv_usec) - ((start->tv_sec*1000000)+start->tv_usec))/1000;

    return time;
}

// void and_seperate(char *command_given, char *list_to_store_commands){
//     char *command = strtok(command_given, "&");
//     int index_of_command = 0;
//     while (command != NULL)
//     {
//         list_to_store_commands[index_of_command] = command;
//         index_of_command++;
//         command = strtok(NULL, "&");
//     }
//     // puttling NULL at the end of the list to identify as a stop.
//     list_to_store_commands[index_of_command] = NULL;
// }
// void pipe_seperate(char *command_given, char *list_to_store_commands){
//     char *command = strtok(command_given, "|");
//     int index_of_command = 0;
//     while (command != NULL)
//     {
//         list_to_store_commands[index_of_command] = command;
//         index_of_command++;
//         command = strtok(NULL, "|");
//     }
//     // puttling NULL at the end of the list to identify as a stop.
//     list_to_store_commands[index_of_command] = NULL;
// }
// void make_args(char *command, char *args[])
// {
//     int counter = 0;
//     char *pch = strtok(command, " ");
//     while (pch != NULL)
//     {
//         args[counter++] = pch;
//         pch = strtok(NULL, " ");
//     }
//     args[counter] = NULL;
// }


// executing the command using execvp
int execute_command(char *args[]){
    // special history command
    if ((strcmp(args[0],"show") == 0) && (strcmp(args[1], "history") == 0)){
        show_history();
        exit(EXIT_SUCCESS);
        return 0;
    }
    int execvp_return = execvp(args[0], args);
    if(execvp_return == -1){
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
}

// split the command by the separator
void command_separate(char *command_given, char *list_to_store_commands[], const char *separator) {
    char *command = strtok(command_given, separator);
    int index_of_command = 0;
    while (command != NULL) {
        list_to_store_commands[index_of_command] = command;
        index_of_command++;
        command = strtok(NULL, separator);
    }
    // Putting NULL at the end of the list to identify as a stop.
    list_to_store_commands[index_of_command] = NULL;
}

// first's output is second's input
int create_process_and_run(char* command_given){
    char * commands_in_pipe[max_commands_in_pipe];
    // pipe_seperate(command_given, commands_in_pipe);
    command_separate(command_given, commands_in_pipe, "|");
    int pipe_descriptor[2];
    int read_end = 0;
    char *arguements_in_given_command[max_number_of_arguments];
    int last_command_index_found=0;


    // Forking the first child
    int status = fork();
    if (status<0){
        perror("Could not forked the first child : ");
        exit(EXIT_FAILURE);   
    } else if (status == 0){
        // // Child process
        // while (commands_in_pipe[last_command_index_found+1] != NULL){
        //     int pipe_status = pipe(pipe_descriptor);
        //     // Forking the next child
        //     int status = fork();
        //     if (status<0){
        //         perror("Could not forked the next child : ");
        //         exit(EXIT_FAILURE);
        //     } else if (status == 0){
        //         // Child process
        //         close(pipe_descriptor[1]);
        //         dup2(read_end, 0);
        //         dup2(pipe_descriptor[0], 0);
        //         close(pipe_descriptor[0]);
        //         read_end = pipe_descriptor[1];
        //         // make_args(commands_in_pipe[last_command_index_found], arguements_in_given_command);
        //         command_separate(commands_in_pipe[last_command_index_found], arguements_in_given_command, " ");
        //         execvp(arguements_in_given_command[0], arguements_in_given_command);
        //         perror("Could not execute the command : ");
        //         exit(EXIT_FAILURE);
        //     } else {
        //         // Parent process
        //         close(pipe_descriptor[0]);
        //         close(read_end);
        //         last_command_index_found++;
        //     }
        // }
        
        //  child process executing the commands
        while (commands_in_pipe[last_command_index_found+1]!=NULL)
        {
            command_separate(commands_in_pipe[last_command_index_found], arguements_in_given_command, " ");
            if (pipe(pipe_descriptor) == -1){
                perror("Error in pipe : ");
                exit(EXIT_FAILURE);
            }
            if (fork() == 0){
                if (read_end != STDIN_FILENO){
                    dup2(read_end, STDIN_FILENO);
                    close(read_end);
                }
                if (pipe_descriptor[1] != STDOUT_FILENO){
                    dup2(pipe_descriptor[1], STDOUT_FILENO);
                    close(pipe_descriptor[1]);
                }
                execute_command(arguements_in_given_command);
                perror("Could not execute the command : ");
                exit(EXIT_FAILURE);
            }
            close(pipe_descriptor[1]);
            read_end = pipe_descriptor[0];
            last_command_index_found++;
        }

        if (read_end != STDIN_FILENO){
            dup2(read_end, STDIN_FILENO);
        }
        command_separate(commands_in_pipe[last_command_index_found], arguements_in_given_command, " ");
        perror("Error : ");
        exit(EXIT_FAILURE);       
    } else{
        // Parent process waits for all child processes and updates history
        int child_pid, child_exit_status;
        
        time_value start,end;
        gettimeofday(&start, 0);

        // waiting for all the child processes to finish
        while(1){
            child_pid = wait(&child_exit_status);
            if(child_pid < 0){
                break;
            } else if(WIFEXITED(child_exit_status)){
                // command executed normally
                gettimeofday(&end, 0);
                unsigned long duration = elapse_time(&start, &end);
                // store_command(child_pid, command_given, start.tv_sec, end.tv_sec, duration);
            } else {
                printf("Abnormal termination of %d\n", child_pid);
            }
        }
    }
    return status;
}

// running multiple commands
int background_process(char *command_given){
    int status;

    char command_copy[max_input_size_of_command];
    strcpy(command_copy, command_given);
    
    char *list_of_commands[max_number_of_background_commands_in_one_command];
    // and_seperate(command_copy, list_of_commands);
    command_separate(command_copy, list_of_commands, "&");
    
    for (int i=0; list_of_commands[i]!=NULL; i++){
        status = create_process_and_run(list_of_arguments[i]);
    }

    return status;
}

// running scripts
int read_file_and_run(char *file_name){
    int status;
    FILE *file_descriptor;
    file_descriptor = fopen(file_name, "r");
    
    if(file_descriptor == NULL){
        perror("File not found\n");
        return 1;
    }
    
    char * line_read = NULL;
    size_t length_of_line = 0;
    
    while (getline_read(&line_read, &length_of_line, file_descriptor)!= -1){
        line_read[strcspn(line_read, "\n")] = 0;
        status = background_process(line_read);
    }

    if (line_read){
        free(line_read);
    }
    

    fclose(file_descriptor);
    return status;
}

// launching the command
int launch(char *command_given){
    int status=0;
    
    // making a structure to store the start and end time of the command
    time_value start, end;

    // getting the start time of the command
    gettimeofday(&start, 0);

    // checking if the command is a file reference
    if (memcmp(command_given, "./", 2) == 0){
        
        // there was a file reference given
        if(memcmp((command_given + strlength_of_line(command_given) - 2), ".sh", 3) == 0){
            
            // it was a .sh file
            status = read_file_and_run(command_given + 2);
        } else { 
            
            // treating it like normal executable
            status = background_process(command_given);
        }
    } else {
        
        // Handling the command as a normal command
        status = background_process(command_given);
    }
    
    // getting the end time of the commmand
    gettimeofday(&end, 0);

    // Storing command in the history table
    store_command(status, command_given, start.tv_sec, end.tv_sec, elapse_time(&start, &end));
    return status;
}

int main(){
    // making a signal handler
    signal(SIGINT, signal_handler);
    int status;
    char command[max_input_size_of_command];

    // initializing the history for readline_read
    using_history();


    do {

        // implementing the readline_read functionality to read the input command
        char *command_buffer = readline_read("vemy@simplishell:~$ ");

        if (command_buffer && *command_buffer){ 

            // to stop the program if memory is not allocated or the command is null
            strcpy(command, command_buffer);
            
            // adding it to the readline_read history
            add_history(command);

        } else {

            // if the command is null, then continue to the next iteration
            status = 1;
            continue;
        }
        
        // removing the newline_read character from the end of the command
        command[strlength_of_line(command) - 1] = 0;

        // executing the command and checking its status
        status = launch(command);
    } while (status);

}