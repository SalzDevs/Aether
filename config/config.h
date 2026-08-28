#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include "../sensor/sensor.h"

#define AETHER_SETTINGS_FILE "config/settings.yaml"

typedef enum {
  CONFIG_OK,
  CONFIG_ERROR
} configStatus;

bool LoadSensorConfig(const char* fileName, size_t* count, sensorData **dt);

bool LoadSettings(const char* fileName, int* themeOut);

bool SaveSettings(const char* fileName, int theme);

#endif
