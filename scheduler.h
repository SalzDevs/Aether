#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

//TODO: add to the struct the void function pointer to have the function that the Task should execute
typedef struct {
  //period of running the task
  uint32_t period;
  // time of the last run
  uint64_t last_run;
} Task;

// Initiate the Task Strucure
void initTask(Task* t, uint32_t period);

//Prints to standard output task values
void printTask(Task task);

//Evaluates if enough time as passed to run Task 
bool shouldRunTask(uint32_t period, uint64_t last_run);

#endif
