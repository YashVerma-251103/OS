// fibbonacci series using recursion

#include <stdio.h>
#include "dummy_main.h"

int fib(int n)
{
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char **argv)
// int main()
{
    int n = 45;
    printf("Fibonacci series of %d numbers is: ", n);
    printf(" %d \n", fib(n));
    return 0;
}
