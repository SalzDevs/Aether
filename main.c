#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "sensor/sensor.h"
#include "Qeue/qeue.h"
#include "scheduler/scheduler.h"
#include "config/config.h"

typedef struct {
  sensorQeue *sq;
  int sensorId;
} taskArgs;

static void mockSensorTask(void *ctx) {
  taskArgs *args = ctx;

  sensorData d;
  initSensorData(&d);
  d.sensorId = args->sensorId;
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

  //Load sensors from config
  size_t sensorCount;
  sensorData* configSensors = fileToSensors("config/sensors.yaml", &sensorCount);
  if (configSensors == NULL) return 1;

  //Initialize queue seeded with the config values
  sensorQeue sq;
  initSensorQeue(&sq);
  for (size_t i = 0; i < sensorCount; i++) {
    addElemToQueue(&sq, configSensors[i]);
  }

  //One task per sensor
  taskArgs* args = malloc(sensorCount * sizeof(taskArgs));
  Task* tasks = malloc(sensorCount * sizeof(Task));
  for (size_t i = 0; i < sensorCount; i++) {
    args[i] = (taskArgs){ .sq = &sq, .sensorId = configSensors[i].sensorId };
    initTask(&tasks[i], 3, mockSensorTask, &args[i], time(NULL));
  }

  free(configSensors);

  int displayBlockHeight = displayBlockHeightCalc(screenHeight, sq.current_size);
  char buf[128];

  int fontSize;
  int textWidth;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    DrawText(TextFormat("Time: %.02f", GetTime()), 0, 0, 10, TIMER_TEXT_COLOR); 

    int padding = 20; 
    int maxWidth = screenWidth - (padding * 2);

    for (size_t i = 0; i < sq.current_size; i++) {
      sensorDataToString(sq.data[i], buf, sizeof(buf));

      int fontSize = displayBlockHeight - 10; 

      textWidth = MeasureText(buf, fontSize);
      while ((textWidth > maxWidth) && (fontSize > 1)) {
        fontSize--;
        textWidth = MeasureText(buf, fontSize);
      }

      int posX = (screenWidth - textWidth) / 2;
      int posY = (displayBlockHeight * i) + (displayBlockHeight - fontSize) / 2;

      DrawText(buf, posX, posY, fontSize, DATA_TEXT_COLOR);
      DrawLine(0, displayBlockHeight * i, screenWidth, displayBlockHeight * i, DIVIDER_COLOR);
    }

    EndDrawing();

    for (size_t i = 0; i < sensorCount; i++) {
      runTask(&tasks[i], &sq);
    }

    printQeue(&sq);
    displayBlockHeight = displayBlockHeightCalc(screenHeight, sq.current_size);
  }

  free(args);
  free(tasks);
  return 0;
}
