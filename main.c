#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "sensor/sensor.h"
#include "Qeue/qeue.h"
#include "scheduler/scheduler.h"

typedef struct {
  sensorQeue *sq;
  sensor *s;
  int sensorIdx;
} taskArgs;

static void mockSensorTask(void *ctx) {
  taskArgs *args = ctx;

  sensorData d;
  initSensorData(&d);
  d.sensorId = args->s->id;
  sensorDataUpdate(&d);
  addElemToQueue(args->sq, d);
}

static int displayBlockHeightCalc(int screenHeight, int amountOfSensors) {
  return screenHeight/amountOfSensors;
} 

#define BACKGROUND_COLOR ((Color){ 24, 26, 32, 255 })
#define TIMER_TEXT_COLOR ((Color){ 150, 158, 172, 255 })
#define DATA_TEXT_COLOR  ((Color){ 110, 190, 255, 255 })
#define DIVIDER_COLOR    ((Color){ 52, 57, 70, 255 })

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

  initSensor(&s1, 1);
  initSensor(&s2, 2);
  initSensor(&s3, 3);
  initSensor(&s4, 4);

  //Reserve a data slot in the queue for each sensor
  sensorData d1;
  sensorData d2;
  sensorData d3;
  sensorData d4;

  initSensorData(&d1);
  initSensorData(&d2);
  initSensorData(&d3);
  initSensorData(&d4);

  d1.sensorId = 1;
  d2.sensorId = 2;
  d3.sensorId = 3;
  d4.sensorId = 4;

  addElemToQueue(&sq, d1);
  addElemToQueue(&sq, d2);
  addElemToQueue(&sq, d3);
  addElemToQueue(&sq, d4);

  taskArgs args1 = {
    .sq = &sq,
    .s = &s1,
    .sensorIdx = 0
  };

  taskArgs args2 = {
    .sq = &sq,
    .s = &s2,
    .sensorIdx = 1
  };

  taskArgs args3 = {
    .sq = &sq,
    .s = &s3,
    .sensorIdx = 2
  };

  taskArgs args4 = {
    .sq = &sq,
    .s = &s4,
    .sensorIdx = 3
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
  char buf[128];
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
    DrawText(TextFormat("Time: %.02f", GetTime()), 0, 0, 10, TIMER_TEXT_COLOR); 
    for (size_t i = 0; i < sq.current_size; i++) {
      sensorDataToString(sq.data[i], buf, sizeof(buf));
      DrawText(buf, screenWidth/2-300, (displayBlockHeight*i)+(displayBlockHeight/2), 15, DATA_TEXT_COLOR);
      DrawLine(0, displayBlockHeight*i, screenWidth, displayBlockHeight*i, DIVIDER_COLOR);
    }

    EndDrawing();
    runTask(&sensorTask1, &sq);
    runTask(&sensorTask2, &sq);
    runTask(&sensorTask3, &sq);
    runTask(&sensorTask4, &sq);
    printQeue(&sq);
    displayBlockHeight = displayBlockHeightCalc(screenHeight, sq.current_size);
  }
  return 0;
}
