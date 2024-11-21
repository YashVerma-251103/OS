#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>

#include <thread>
#include <sys/time.h>

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

#define main user_main

/* Definitions for better code understanding */
#define ptr *
#define address_of(object) &object
#define offset int
#define limit int
#define single_parallel_task std::function<void(int)>
#define double_parallel_task std::function<void(int, int)>
#define timeval struct timeval
#define thread pthread_t
#define chunk int
#define index int
#define last_index 0
#define second_last_index 1
#define range int

/* Structures for arguments for threads */
struct arg_single_task
{
  limit upper_limit;
  limit lower_limit;
  single_parallel_task ptr function;
};
struct arg_double_task
{
  offset x_offset;
  offset y_offset;
  limit upper_limit;
  limit lower_limit;
  int size;
  double_parallel_task ptr function;
};

/* Typedefs */
#define st_arg struct arg_single_task
#define dt_arg struct arg_double_task

/* Macros for loops */
#define limits_loop for (index i = lower; i < upper; i++)
#define loop_run_for_total_threads(last_index) for (index i = 0; i < number_of_threads - last_index; i++)

/* Helper Functions */
// single for loop
void ptr run_parallel_single_for_loop(void ptr args)
{
  single_parallel_task ptr function = (single_parallel_task ptr)(((st_arg ptr)args)->function);
  limit upper = ((st_arg ptr)args)->upper_limit;
  limit lower = ((st_arg ptr)args)->lower_limit;

  limits_loop(ptr(function))(i);

  return NULL;
}

// double for loops
void ptr run_parallel_double_for_loop(void ptr args)
{
  double_parallel_task ptr function = (double_parallel_task ptr)(((dt_arg ptr)args)->function);

  offset x_offset = ((dt_arg ptr)args)->x_offset;
  offset y_offset = ((dt_arg ptr)args)->y_offset;
  limit upper = ((dt_arg ptr)args)->upper_limit;
  limit lower = ((dt_arg ptr)args)->lower_limit;
  int size = ((dt_arg ptr)args)->size;

  limits_loop(ptr(function))((i / size + x_offset), (i % size + y_offset));

  return NULL;
}

/* Function to run parallel for loop */
// single for loop runner
void parallel_for(int low, int high, single_parallel_task &&lambda, int number_of_threads)
{
  timeval start, end; // time values for getting duration

  gettimeofday(address_of(start), NULL);

  thread threads[number_of_threads - 1];
  st_arg args[number_of_threads];

  chunk chunk_size = (high - low) / number_of_threads;

  loop_run_for_total_threads(last_index)
  {
    args[i].upper_limit = ((i + 1) * chunk_size);
    args[i].lower_limit = i * chunk_size;
    args[i].function = address_of(lambda);
    if (i == number_of_threads - 1)
    {
      args[i].upper_limit = high;
    }
  }

  // constructing threads
  loop_run_for_total_threads(second_last_index)
      pthread_create(address_of(threads[i]), NULL, run_parallel_single_for_loop, (void ptr) address_of(args[i]));

  // main thread
  run_parallel_single_for_loop((void ptr) address_of(args[number_of_threads - 1]));

  // joining threads
  loop_run_for_total_threads(second_last_index)
      pthread_join(threads[i], NULL);

  gettimeofday(address_of(end), NULL);

  printf("Total Execution Time: %f\n", (((double)(end.tv_usec - start.tv_usec) / 1000000) + ((double)(end.tv_sec - start.tv_sec))));
}

// double for loop runner
void parallel_for(int low_1, int high_1, int low_2, int high_2, double_parallel_task &&lambda, int number_of_threads)
{
  timeval start, end; // time values for getting duration

  gettimeofday(address_of(start), NULL);

  range r1 = (high_1 - low_1), r2 = (high_2 - low_2);
  chunk chunk_size = ((r1 * r2) / number_of_threads);

  thread threads[number_of_threads - 1];
  dt_arg args[number_of_threads];

  loop_run_for_total_threads(last_index)
  {
    args[i].upper_limit = ((i + 1) * chunk_size);
    args[i].lower_limit = i * chunk_size;
    args[i].function = address_of(lambda);
    args[i].size = r1;
    args[i].x_offset = low_1;
    args[i].y_offset = low_2;
    if (i == number_of_threads - 1)
    {
      args[i].upper_limit = (r1 * r2);
    }
  }

  // constructing threads
  loop_run_for_total_threads(second_last_index)
      pthread_create(address_of(threads[i]), NULL, run_parallel_double_for_loop, (void ptr) address_of(args[i]));

  // main thread
  run_parallel_double_for_loop((void ptr) address_of(args[number_of_threads - 1]));

  // joining threads
  loop_run_for_total_threads(second_last_index)
      pthread_join(threads[i], NULL);

  gettimeofday(address_of(end), NULL);

  printf("Total Execution Time: %f\n", (((double)(end.tv_usec - start.tv_usec) / 1000000) + ((double)(end.tv_sec - start.tv_sec))));
}
