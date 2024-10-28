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

// make a structure to store history
struct commands
{
    pid_t pid;
    char command[128];
    int wait_time;
    int execution_time;
};

// process structure to store pid and command
struct process
{
    pid_t pid;
    char command[128];
    int num_slice;
    int wait_time;
    struct timeval last_exec;
    struct process *next;
};

// shm struct for shared memory
typedef struct shm_t
{
    char command[128];
    int priority;
    sem_t mutex;
} shm_t;

int fd;

struct commands history[100];
int history_index = 0;
long int NCPU;
long int TSLICE;

sigset_t signal_mask;
shm_t *shm;

void cleanup(shm_t *shm);
// shows history
void show_history()
{
    for (int i = 0; i < history_index; i++)
    {
        printf("%s\n", history[i].command);
    }
}

// function for sleep
void msleep(long msec)
{
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
}

// add the data to history struct
void add_to_history(pid_t pid, char *command, int wait, int exec)
{
    history[history_index].pid = pid;
    strcpy(history[history_index].command, command);
    history[history_index].wait_time = wait;
    history[history_index].execution_time = exec;
    history_index++;
}

// to find end time for a command
unsigned long end_time(struct timeval *start)
{
    struct timeval end;
    unsigned long t;

    gettimeofday(&end, 0);
    t = ((end.tv_sec * 1000000) + end.tv_usec) -
        ((start->tv_sec * 1000000) + start->tv_usec);
    return t / 1000;
}

// enqueue processes
struct process *enqueue(struct process *queue, struct process *p)
{
    if (queue == NULL)
    {
        p->next = NULL;
        return p;
    }
    queue->next = enqueue(queue->next, p);
    return queue;
}

// remove terminated processes
struct process *dequeue_enqueue_processes(struct process *running, struct process *queue)
{
    if (running == NULL)
    {
        return queue;
    }
    queue = dequeue_enqueue_processes(running->next, queue);
    if (waitpid(running->pid, NULL, WNOHANG) != 0)
    {
        add_to_history(running->pid, running->command, running->wait_time, running->num_slice * TSLICE); // TODO
        free(running);
        return queue;
    }
    return enqueue(queue, running);
}

// continue execution of processes
void continue_running(struct process *running)
{
    if (running == NULL)
    {
        return;
    }
    running->wait_time += end_time(&running->last_exec);
    running->num_slice++;
    kill(running->pid, SIGCONT);
    continue_running(running->next);
}

// pause execution of processes
void pause_processes(struct process *running)
{
    if (running == NULL)
    {
        return;
    }
    kill(running->pid, SIGSTOP);
    gettimeofday(&running->last_exec, 0);
    pause_processes(running->next);
}

// cycle all processes
struct process *cycle_processes(struct process *queue)
{
    struct process *running = NULL;
    while (queue)
    {
        for (int i = 0; (i < NCPU) && (queue != NULL); i++)
        {
            struct process *next = queue->next;
            running = enqueue(running, queue);
            queue = next;
        }
        continue_running(running);
        msleep(TSLICE);
        pause_processes(running);
        queue = dequeue_enqueue_processes(running, queue);
        running = NULL;
    }
    return queue;
} // cycle_process

// signal handler to end of input
static void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        sem_wait(&shm->mutex);
        shm->priority = -2;
        sem_post(&shm->mutex);
        wait(NULL);

        cleanup(shm);

        exit(signum);
    }
}

// signal handler for childrens
static void dummy_signal_handler(int signum)
{
    if (signum == SIGINT)
    {
    }
}

// allocate new process
struct process *allocate_process(struct process *queue, char name[])
{
    if (queue == NULL)
    {
        queue = (struct process *)malloc(sizeof(struct process));
        gettimeofday(&queue->last_exec, 0);
        sigprocmask(SIG_BLOCK, &signal_mask, NULL);
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            exit(0);
        }
        if (pid == 0)
        {
            signal(SIGINT, dummy_signal_handler);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            execlp(name, name, NULL);
            perror("Exec");
            exit(0);
        }
        else
        {
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            strcpy(queue->command, name);
            kill(pid, SIGSTOP);
        }
        queue->pid = pid;
        return queue;
    }
    queue->next = allocate_process(queue->next, name);
    return queue;
}

// splits the commands and runs them through pipes
void make_pipe_commands(char *command, char *pipe_commands[])
{
    int counter = 0;
    char *pipe_command = strtok(command, "|");
    while (pipe_command != NULL)
    {
        pipe_commands[counter++] = pipe_command;
        pipe_command = strtok(NULL, "|");
    }
    pipe_commands[counter] = NULL;
}

// convert command to arguments
void make_args(char *command, char *args[])
{
    int counter = 0;
    char *pch = strtok(command, " ");
    while (pch != NULL)
    {
        args[counter++] = pch;
        pch = strtok(NULL, " ");
    }
    args[counter] = NULL;
}

// runs the command using the above functions
void run_command(char *args[])
{
    if (strcmp(args[0], "history") == 0)
    {
        show_history();
        exit(0);
    }

    execvp(args[0], args);
}

// give output of first process as input of second
int create_process_and_run(char *command)
{
    char *args[64];
    char *pipe_commands[16];
    make_pipe_commands(command, pipe_commands);
    int pd[2], input_pipe = 0;
    int i;

    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened: %d\n", status);
        exit(0);
    }
    else if (status == 0)
    {
        for (i = 0; pipe_commands[i + 1] != NULL; i++)
        {
            make_args(pipe_commands[i], args);
            if (pipe(pd) == -1)
            {
                perror("Error in pipe");
                exit(1);
            }
            if (fork() == 0)
            {
                if (input_pipe != STDIN_FILENO)
                {
                    dup2(input_pipe, STDIN_FILENO);
                    close(input_pipe);
                }
                if (pd[1] != STDOUT_FILENO)
                {
                    dup2(pd[1], STDOUT_FILENO);
                    close(pd[1]);
                }
                run_command(args);
                perror("Error");
                exit(1);
            }

            close(pd[1]);

            input_pipe = pd[0];
        }

        if (input_pipe != STDIN_FILENO)
        {
            dup2(input_pipe, STDIN_FILENO);
        }

        make_args(pipe_commands[i], args);

        run_command(args);
        perror("Error");
        exit(1);
    }
    else
    {
        int ret;
        int pid = wait(&ret);
        if (WIFEXITED(ret))
        {
            // printf("%d Exit =%d\n", pid, WEXITSTATUS(ret));
        }
        else
        {
            printf("Abnormal termination of %d\n", pid);
        }
        // printf("I am the parent process\n");
    }

    return status;
}

// get user input
void read_user_input(char *input)
{
    fgets(input, 128, stdin);
    input[strlen(input) - 1] = 0;
}

// launch the command using above function and add to history
int launch(char *command)
{
    int status = 0;
    struct timeval start;

    gettimeofday(&start, 0);

    char *args[12];
    char temp[strlen(command) + 1];
    strcpy(temp, command);
    make_args(temp, args);

    if (strcmp(args[0], "submit") == 0)
    {
        int priority;
        if (args[1] == NULL)
        {
            printf("Submit failed\n");
            return 1;
        }
        if (args[2] == NULL)
        {
            priority = 1;
        }
        else if (args[3] != NULL)
        {
            printf("Incorrect number of arguments\n");
            return 1;
        }
        else
        {
            priority = atoi(args[2]);
        }
        if ((priority >= 1) && (priority <= 4))
        {
            sem_wait(&shm->mutex);
            strcpy(shm->command, args[1]);
            shm->priority = priority;
            sem_post(&shm->mutex);
        }
        else
        {
            printf("priority out of range\n");
        }
        return 1;
    }

    if (strcmp(args[0], "schedular") == 0)
    {
        sem_wait(&shm->mutex);
        shm->priority = -1;
        sem_post(&shm->mutex);
        return 1;
    }

    status = create_process_and_run(command);

    return status;
}

// run one cycle TSLICE
struct process *next_cycle(struct process *queue)
{
    if (queue == NULL)
        return NULL;
    struct process *running = NULL;
    for (int i = 0; (i < NCPU) && (queue != NULL); i++)
    {
        struct process *next = queue->next;
        running = enqueue(running, queue);
        queue = next;
    }
    continue_running(running);
    msleep(TSLICE);
    pause_processes(running);
    queue = dequeue_enqueue_processes(running, queue);
    running = NULL;
    return queue;
}

// clean not running processes
struct process *clean_not_running(struct process *queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    if (waitpid(queue->pid, NULL, WNOHANG) != 0)
    {
        return queue->next;
    }
    queue->next = clean_not_running(queue->next);
    return queue;
}

// setup the shared memory
shm_t *setup()
{
    fd = shm_open("schedular", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        printf("Error in file\n");
        exit(0);
    }
    if (ftruncate(fd, sizeof(shm_t)) == -1)
    {
        printf("Error in ftruncate\n");
        exit(0);
    }

    shm_t *map = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        printf("Map failed\n");
        exit(0);
    }

    return map;
}

// clean the shared memory
void cleanup(shm_t *shm)
{
    if (sem_destroy(&shm->mutex) == -1)
    {
        printf("Error in sem_destroy\n");
        exit(0);
    }
    if (munmap(shm, sizeof(struct shm_t)) == -1)
    {
        printf("Error in munmap\n");
        exit(0);
    }
    if (close(fd) == -1)
    {
        printf("Error in closing file\n");
        exit(0);
    }
    if (shm_unlink("schedular") == -1)
    {
        printf("Unlink failed\n");
        exit(0);
    }
}

// clean shared memory for child
void close_exit(shm_t *shm)
{
    if (munmap(shm, sizeof(struct shm_t)) == -1)
    {
        printf("Error in munmap\n");
        exit(0);
    }
    if (close(fd) == -1)
    {
        printf("Error in closing file\n");
        exit(0);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    // signal handler
    signal(SIGINT, signal_handler);
    if (argc != 3)
    {
        printf("Usage: simple-shell NCPU TSLICE\n");
        exit(0);
    }

    NCPU = strtol(argv[1], NULL, 10);
    TSLICE = strtol(argv[2], NULL, 10);

    if (!((NCPU) && (TSLICE))) {
        printf("Usage: simple-shell NCPU TSLICE\n");
        exit(0);
    }

    shm = setup();
    shm->priority = 0;
    shm->command[0] = '\0';
    sem_init(&shm->mutex, 1, 1);

    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    pid_t schedular_pid = fork();

    if (schedular_pid < 0)
    {
        perror("fork");
        exit(0);
    }
    else if (schedular_pid == 0)
    {
        // schedular
        signal(SIGINT, dummy_signal_handler);
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        struct process *queue[] = {NULL, NULL, NULL, NULL};
        char command[128];
        int priority = 0;
        int start_running = 0;
        while (1)
        {
            sem_wait(&shm->mutex);
            priority = shm->priority;
            shm->priority = 0;
            strcpy(command, shm->command);
            sem_post(&shm->mutex);

            if (!start_running)
            {
                for (int i = 0; i < 4; i++)
                {
                    queue[i] = clean_not_running(queue[i]);
                }
            }

            if (priority > 0)
            {
                queue[priority - 1] = allocate_process(queue[priority - 1], command);
            }
            else if (priority == -1)
            {
                start_running = 1;
            }
            else if (priority == -2)
            {
                for (int i = 0; i < 4; i++)
                {
                    queue[i] = cycle_processes(queue[i]);
                }
                break;
            }
            if (start_running)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (queue[i])
                    {
                        queue[i] = next_cycle(queue[i]);
                        break;
                    }
                }
                if ((queue[0] == NULL) && (queue[1] == NULL) && (queue[2] == NULL) && (queue[3] == NULL))
                {
                    start_running = 0;
                }
            }
        }
        printf("\n");
        // history
        printf("%5s\t%64s\t%10s\t%12s\n", "PID", "Command", "Exec Time", "Wait time(ms)");
        for (int i = 0; i < history_index; i++)
        {
            printf("%5d\t%64s\t%10d\t%12d\n", history[i].pid, history[i].command, history[i].execution_time, history[i].wait_time);
        }
        close_exit(shm);
    }
    else
    {
        sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
        int status;
        char command[128];
        do
        {
            printf("$ ");
            fflush(stdout);
            read_user_input(command);
            status = launch(command);
        } while (status);
    }
}
