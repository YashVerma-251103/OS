#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <thread>
#include <sys/time.h>


// structs for arguments for threads

struct arg1
{
  int ulimit;
  int llimit;
  std::function<void(int)> *function;
};

struct arg2
{
  int xoffset;
  int yoffset;
  int ulimit;
  int llimit;
  int size;
  std::function<void(int, int)> *function;
};

int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> &&lambda)
{
  lambda();
}

int main(int argc, char **argv)
{
  /*
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be
   * explicity captured if they are used inside lambda.
   */
  int x = 5, y = 1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/ [/*by value*/ x, /*by reference*/ &y](void)
  {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout << "====== Welcome to Assignment-" << y << " of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);

  auto /*name*/ lambda2 = [/*nothing captured*/]()
  {
    std::cout << "====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

// for running single for loop
void *run_parallel_for1(void *args)
{
  std::function<void(int)> *function = (std::function<void(int)> *)(((struct arg1 *)args)->function);
  int ulimit = ((struct arg1 *)args)->ulimit;
  int llimit = ((struct arg1 *)args)->llimit;
  for (int j = llimit; j < ulimit; j++)
  {
    (*(function))(j);
  }
  return NULL;
}

// main function for running single for loop
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads)
{
  struct timeval tv1, tv2; //for duration

  gettimeofday(&tv1, NULL);

  pthread_t threads[numThreads - 1];
  struct arg1 args[numThreads];

  int chunk = (high - low) / numThreads;

  for (int i = 0; i < numThreads; i++)
  {
    args[i].ulimit = ((i + 1) * chunk);
    args[i].llimit = i * chunk;
    args[i].function = &lambda;
    if (i == numThreads - 1) {
      args[i].ulimit = high;
    }
  }

  for (int i = 0; i < numThreads - 1; i++)
  {
    pthread_create(&threads[i], NULL, run_parallel_for1, (void *)&args[i]); // making threads
  }

  run_parallel_for1((void *)&args[numThreads - 1]); // for main thread

  for (int i = 0; i < numThreads - 1; i++)
  {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&tv2, NULL);

  printf("Execution time: %f\n", (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double)(tv2.tv_sec - tv1.tv_sec));
}

// helper function for running double for loops
void *run_parallel_for2(void *args)
{
  std::function<void(int, int)> *function = (std::function<void(int, int)> *)(((struct arg2 *)args)->function);
  int ulimit = ((struct arg2 *)args)->ulimit;
  int llimit = ((struct arg2 *)args)->llimit;
  int size = ((struct arg2 *)args)->size;
  int xoffset = ((struct arg2 *)args)->xoffset;
  int yoffset = ((struct arg2 *)args)->yoffset;
  for (int j = llimit; j < ulimit; j++)
  {
    (*(function))(j / size + xoffset, j % size + yoffset);
  }
  return NULL;
}

// main function for running double for loops
void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads)
{
  struct timeval tv1, tv2; // time values for getting duration

  gettimeofday(&tv1, NULL);

  int range1 = (high1 - low1);
  int range2 = (high2 - low2);
  int chunk = (range1 * range2) / numThreads;

  pthread_t threads[numThreads - 1];
  struct arg2 args[numThreads];

  for (int i = 0; i < numThreads; i++)
  {
    args[i].ulimit = ((i + 1) * chunk);
    args[i].llimit = i * chunk;
    args[i].function = &lambda;
    args[i].size = range1;
    args[i].xoffset = low1;
    args[i].yoffset = low2;
    if (i == numThreads - 1) {
      args[i].ulimit = (range1 * range2);
    }
  }

  for (int i = 0; i < numThreads - 1; i++)
  {
    pthread_create(&threads[i], NULL, run_parallel_for2, (void *)&args[i]); // running the threads
  }

  run_parallel_for2((void *)&args[numThreads - 1]); // for main thread

  for (int i = 0; i < numThreads - 1; i++)
  {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&tv2, NULL);

  printf("Execution time: %f\n", (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double)(tv2.tv_sec - tv1.tv_sec));
}

#define main user_main
