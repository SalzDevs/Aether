#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "sensor/sensor.h"
#include "Qeue/qeue.h"
#include "scheduler/scheduler.h"


static void mockSensorTask(taskCtx* ctx) {
  sensorDataUpdate(ctx->s);
  addElemToQueue(ctx->q, *ctx->s);
}

int main() {
  srand(time(NULL));
  
  //initialize qeue
  sensorQeue sq;
  initSensorQeue(&sq);
  
  //initialize sensor
  sensor s;
  initSensor(&s);
  Task sensor_task;
  taskCtx sensor_task_ctx;
  initTaskCtx(&sensor_task_ctx, &sq, &s);
  initTask(&sensor_task, 3, mockSensorTask, &sensor_task_ctx, time(NULL));

  while (1) {
    runTask(&sensor_task);
    if (sq.current_size > 0) {
      printQeue(&sq);
    }
    sleep(1);
  }
  return 0;
}
