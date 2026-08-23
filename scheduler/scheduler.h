#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../sensor/sensor.h"
#include "../Queue/queue.h"

typedef struct {
  //period of running the task
  uint32_t period;
  // time of the last run
  uint64_t last_run;
  // executable task function
  void (*function)(void*);
  // arguments of the executable task
  void* ctx;
} Task;


// Initiate the Task Strucure
void initTask(Task* t, uint32_t period, void (*function)(void*), void* ctx, uint64_t current_time);

//Prints to standard output task values
void printTask(Task task);

//Evaluates if enough time as passed to run Task 
bool shouldRunTask(uint32_t period, uint64_t last_run, uint64_t current_time);

// Runs the task if the period as passed
void runTask(Task* t, sensorQueue* sq); 

#endif
