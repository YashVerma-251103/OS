// /* Summary
// You have to implement a SimpleShell that waits for user input, executes commands provided in the user input, and then repeats until terminated using ctrl-c. You don't have to implement the individual Unix commands that will execute on your SimpleShell. */

// /* Instructions
// 2. SimpleShell Implementation (“simple-shell.c”)
// We are not providing any starter code for this assignment, but a detailed description is provided herewith for
// your implementation:
//     a. The main job of any shell (including SimpleShell) is to read the user command from the standard input, parse the command string (command and its arguments if any), and execute the command along with the command line arguments provided by the user. All these three steps should be carried out in an infinite do-while loop as shown in the Lecture 06 slides.
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
//     The “fib” and “helloworld” are the executables of a Fibonacci number calculator and hello world programs respectively (c-code) that would be made available in the directory where your simple-shell.c will reside. The file “file.txt” is some file that you can create with repetitive lines to test “uniq” command. Note that you might have to know the location of the ELF file for the Unix commands. The “which” command is helpful that would show you that the commands are stored inside the directory /usr/bin
//     f. The concepts and system calls discussed in Lecture 06 and 07 is required for the implementation of your SimpleShell. It should also support pipes.
//     g. SimpleShell should also support history command that should only show the commands entered on the SimpleShell command prompt (along with their command line arguments).
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
//     An exec calls the OS loader internally that loads the ELF file with its command line argument as specified in the argument list
//     */


/* what to do?
    - Command is entered and if length is non-null, keep it in history.
    - Parsing : Parsing is the breaking up of commands into individual words and strings
    - Checking for special characters like pipes, etc is done
    - Checking if built-in commands are asked for.
    - If pipes are present, handling pipes.
    - Executing system commands and libraries by forking a child and calling execvp.
    - Printing current directory name and asking for next input.

For keeping history of commands, recovering history using arrow keys and handling autocomplete using the tab key, we will be using the readline library provided by GNU.
*/


// Libraries
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(){
    
}