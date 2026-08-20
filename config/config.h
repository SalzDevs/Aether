#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include "../sensor/sensor.h"

typedef enum {
  CONFIG_OK,
  CONFIG_ERROR
} configStatus;

bool LoadSensorConfig(const char* fileName, size_t* count, sensorData **dt);
#endif
