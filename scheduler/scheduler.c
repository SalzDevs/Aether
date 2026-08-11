#include "scheduler.h"
#include "../Qeue/qeue.h"
#include <stdint.h>
#include <stdio.h>

void initTaskCtx(taskCtx *ctx,sensorQeue* q, sensor* s) {
  ctx->q = q;
  ctx->s = s;
}

void initTask(Task* t, uint32_t period, void (*function)(taskCtx*),taskCtx *ctx, uint64_t current_time) {
  if (ctx==NULL) {
    fprintf(stderr,"To initialize a task you first need to initialze the context related to that task!");
    return;  
  }
  t->period = period;
  t->last_run = current_time;
  t->function = function;
  t->ctx = ctx;
}

void printTask(Task task) {
  printf("Task Period:%u\n", task.period);
  printf("Task Last Run:%llu\n", task.last_run);
}

void runTask(Task* t) {
  if (shouldRunTask(t->period, t->last_run, (uint64_t)time(NULL))) {
    t->function(t->ctx);
    t->last_run = time(NULL);
  }
}

bool shouldRunTask(uint32_t period, uint64_t last_run, uint64_t current_time) {
  return current_time - last_run >= period;
}
