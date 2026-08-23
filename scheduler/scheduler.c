#include "scheduler.h"
#include "../Queue/queue.h"
#include <stdint.h>
#include <stdio.h>

void initTask(Task* t, uint32_t period, void (*function)(void*), void *ctx, uint64_t current_time) {
  t->period = period;
  t->last_run = current_time;
  t->function = function;
  t->ctx = ctx;
}

void printTask(Task task) {
  printf("Task Period:%u\n", task.period);
  printf("Task Last Run:%llu\n", task.last_run);
}

void runTask(Task* t, sensorQueue* sq) {
  if (t == NULL || sq == NULL || t->function == NULL) {
    return;
  }

  uint64_t currentTime = (uint64_t)time(NULL);

  if (shouldRunTask(t->period, t->last_run, currentTime)) {
    t->function(t->ctx);
    t->last_run = currentTime;
    removeElemFromQueue(sq);
  }
}

bool shouldRunTask(uint32_t period, uint64_t last_run, uint64_t current_time) {
  return current_time - last_run >= period;
}
