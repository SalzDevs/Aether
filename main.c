#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
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

static int displayBlockHeightCalc(int screenHeight, int amountOfSensors) {
  return screenHeight/amountOfSensors;
} 

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;
  
  InitWindow(screenWidth, screenHeight, "Aether");
  SetTargetFPS(60);
  
  srand(time(NULL));

  //Initialize queue
  sensorQeue sq;
  initSensorQeue(&sq);

  //Initialize sensors
  sensor s1;
  sensor s2;
  sensor s3;
  sensor s4;

  initSensor(&s1);
  initSensor(&s2);
  initSensor(&s3);
  initSensor(&s4);

  addElemToQueue(&sq, s1);
  addElemToQueue(&sq, s2);
  addElemToQueue(&sq, s3);
  addElemToQueue(&sq, s4);

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

  
  taskArgs args4 = {
    .sq = &sq,
    .sensorIdx = 2
  };

  // Create tasks
  Task sensorTask1;
  Task sensorTask2;
  Task sensorTask3;
  Task sensorTask4;

  initTask(&sensorTask1, 3, mockSensorTask, &args1,time(NULL));
  initTask(&sensorTask2, 3, mockSensorTask, &args2,time(NULL));
  initTask(&sensorTask3, 3, mockSensorTask, &args3,time(NULL));
  initTask(&sensorTask4, 3, mockSensorTask, &args4,time(NULL));
  
  int displayBlockHeight = displayBlockHeightCalc(screenHeight,sq.current_size);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    for (size_t i = 0; i < sq.current_size; i++) {
      DrawLine(0, displayBlockHeight*i, screenWidth, displayBlockHeight*i, RED);
    }
    EndDrawing();
    runTask(&sensorTask1);
    runTask(&sensorTask2);
    runTask(&sensorTask3);
    printQeue(&sq);
  }

  return 0;
}
