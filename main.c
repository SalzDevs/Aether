#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "sensor/sensor.h"
#include "History/history.h"
#include "scheduler/scheduler.h"
#include "config/config.h"
#include "ui/dashboard.h"

#define HISTORY_CAPACITY 64

typedef struct {
  sensorData* sensor;    // entry in the registry, updated in place
  sensorHistory* hist;   // this sensor's reading history
} taskArgs;

static void mockSensorTask(void *ctx) {
  taskArgs *args = ctx;

  sensorDataUpdate(args->sensor, (float)GetTime());
  pushSensorReading(args->hist, *args->sensor);
}

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "Aether");
  SetTargetFPS(60);
  InitDashboard();

  srand(time(NULL));

  // Load sensors from config; the array stays alive and acts as the
  // registry: the single, authoritative copy of each sensor's state.
  size_t sensorCount = 0;
  sensorData* sensors = NULL;
  if (!LoadSensorConfig("config/sensors.yaml", &sensorCount, &sensors)) return 1;

  // One reading history per sensor
  sensorHistory* histories = malloc(sensorCount * sizeof(sensorHistory));
  for (size_t i = 0; i < sensorCount; i++) {
    initSensorHistory(&histories[i], HISTORY_CAPACITY);
  }

  // One task per sensor, updating its registry entry in place
  taskArgs* args = malloc(sensorCount * sizeof(taskArgs));
  Task* tasks = malloc(sensorCount * sizeof(Task));
  for (size_t i = 0; i < sensorCount; i++) {
    args[i] = (taskArgs){ .sensor = &sensors[i], .hist = &histories[i] };
    initTask(&tasks[i], 3, mockSensorTask, &args[i], time(NULL));
  }

  while (!WindowShouldClose()) {
    DrawDashboard(sensors, sensorCount, screenWidth, screenHeight);

    for (size_t i = 0; i < sensorCount; i++) {
      runTask(&tasks[i]);
    }
  }

  for (size_t i = 0; i < sensorCount; i++) {
    destroySensorHistory(&histories[i]);
  }
  free(histories);
  free(args);
  free(tasks);
  free(sensors);
  UnloadDashboard();
  CloseWindow();
  return 0;
}
