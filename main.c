#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sensor/sensor.h"
#include "Qeue/qeue.h"
#include "scheduler/scheduler.h"

typedef struct {
  sensorQeue *sq;
  int sensorIdx;
} taskArgs;

static void mockSensorTask(void *ctx) {
  taskArgs *args = ctx;

  sensorDataUpdate(&args->sq->data[args->sensorIdx]);
}

int main(void) { 
  srand(time(NULL));

  //Initialize queue
  sensorQeue sq;
  initSensorQeue(&sq);

  //Initialize sensors
  sensor s1;
  sensor s2;
  sensor s3;

  initSensor(&s1);
  initSensor(&s2);
  initSensor(&s3);

  addElemToQueue(&sq, s1);
  addElemToQueue(&sq, s2);
  addElemToQueue(&sq, s3);

  taskArgs args1 = {
    .sq = &sq,
    .sensorIdx = 0
  };

  taskArgs args2 = {
    .sq = &sq,
    .sensorIdx = 1
  };

  taskArgs args3 = {
    .sq = &sq,
    .sensorIdx = 2
  };

  // Create tasks
  Task sensorTask1;
  Task sensorTask2;
  Task sensorTask3;

  initTask(&sensorTask1, 3, mockSensorTask, &args1,time(NULL));
  initTask(&sensorTask2, 3, mockSensorTask, &args2,time(NULL));
  initTask(&sensorTask3, 3, mockSensorTask, &args3,time(NULL));

  while (1) {
    runTask(&sensorTask1);
    runTask(&sensorTask2);
    runTask(&sensorTask3);
    printQeue(&sq);
  }

  return 0;
}
