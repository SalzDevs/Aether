#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "raylib.h"
#include "sensor/sensor.h"
#include "Qeue/qeue.h"
#include "scheduler/scheduler.h"


static void mockSensorTask(taskCtx* ctx) {
  sensorDataUpdate(ctx->s);
  addElemToQueue(ctx->q, *ctx->s);
}

int main() {
  const int screenWidht = 800;
  const int screenHeight = 450;

  InitWindow(screenWidht, screenHeight, "raylib is here to stay");
  
  SetTargetFPS(60);

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

  while (!WindowShouldClose()) {
    runTask(&sensor_task);
    if (sq.current_size > 0) {
      printQeue(&sq);
    }
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("HI how are you?", 190, 200, 20, LIGHTGRAY);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
