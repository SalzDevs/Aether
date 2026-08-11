#include "scheduler.h"
#include <stdint.h>
#include <stdio.h>

void initTask(Task* t, uint32_t period, void (*function)(void), uint64_t current_time) {
  t->period = period;
  t->last_run = current_time;
  t->function = function;
}

void printTask(Task task) {
  printf("Task Period:%u\n", task.period);
  printf("Task Last Run:%llu\n", task.last_run);
}

void runTask(Task* t) {
  if (shouldRunTask(t->period, t->last_run, (uint64_t)time(NULL))) {
    t->function();
    t->last_run = time(NULL);
  }
}

bool shouldRunTask(uint32_t period, uint64_t last_run, uint64_t current_time) {
  return current_time - last_run >= period;
}
