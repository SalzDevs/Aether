#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "sensor/sensor.h"
#include "Queue/queue.h"
#include "scheduler/scheduler.h"
#include "config/config.h"
#include "ui/dashboard.h"

typedef struct {
  sensorQueue *sq;
  sensorData data;
} taskArgs;

static void mockSensorTask(void *ctx) {
  taskArgs *args = ctx;

  sensorDataUpdate(&args->data, (float)GetTime());
  addElemToQueue(args->sq, args->data);
}

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "Aether");
  SetTargetFPS(60);
  InitDashboard();

  srand(time(NULL));

  //Load sensors from config
  size_t sensorCount;
  sensorData* configSensors = NULL;
  bool validConfig = LoadSensorConfig("config/sensors.yaml", &sensorCount, &configSensors);
  if (!validConfig) return 1;

  //Initialize queue seeded with the config values
  sensorQueue sq;
  initSensorQueue(&sq);
  for (size_t i = 0; i < sensorCount; i++) {
    addElemToQueue(&sq, configSensors[i]);
  }

  //One task per sensor, each owning its persistent simulated state
  taskArgs* args = malloc(sensorCount * sizeof(taskArgs));
  Task* tasks = malloc(sensorCount * sizeof(Task));
  for (size_t i = 0; i < sensorCount; i++) {
    args[i] = (taskArgs){ .sq = &sq, .data = configSensors[i] };
    initTask(&tasks[i], 3, mockSensorTask, &args[i], time(NULL));
  }

  free(configSensors);

  while (!WindowShouldClose()) {
    DrawDashboard(&sq, screenWidth, screenHeight);

    for (size_t i = 0; i < sensorCount; i++) {
      runTask(&tasks[i], &sq);
    }

    printQueue(&sq);
  }

  free(args);
  free(tasks);
  destroySensorQueue(&sq);
  UnloadDashboard();
  CloseWindow();
  return 0;
}
