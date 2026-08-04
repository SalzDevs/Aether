#include "scheduler.h"
#include <stdint.h>
#include <stdio.h>

void initTask(Task* t, uint32_t period) {
  time_t currentTime;
  time(&currentTime);

  t->period = period;
  t->last_run = currentTime; 
}

void printTask(Task task) {
  printf("Task Period:%u\n", task.period);
  printf("Task Last Run:%llu\n", task.last_run);
}

bool shouldRunTask(uint32_t period, uint64_t last_run) {
  time_t currentTime;
  time(&currentTime);
  return last_run + period >= (uint64_t)currentTime ? true : false;
}
