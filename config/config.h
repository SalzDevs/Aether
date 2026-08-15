#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "../sensor/sensor.h"

typedef enum {
  CONFIG_OK,
  CONFIG_ERROR
} configStatus;

sensorData* fileToSensors(char* fileName, size_t* count);

#endif