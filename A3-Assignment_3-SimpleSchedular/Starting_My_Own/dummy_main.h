#include <signal.h>
#include <unistd.h>

static void signal_handler(int signum)
{
    if (signum == SIGSTOP)
    {
        pause();
    }
    if (signum == SIGCONT) {
        printf("found sigcont\n");
    }
    if (signum == SIGINT) {
    	
    }
}
int dummy_main(int argc, char **argv);
int main(int argc, char **argv)
{
    
    signal(SIGSTOP, signal_handler);
    //signal(SIGCONT, signal_handler);
    signal(SIGINT, signal_handler);

    int ret = dummy_main(argc, argv);
    return ret;
}
#define main dummy_main
