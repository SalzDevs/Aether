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

typedef struct {
  int theme;
  
  bool showSparklines;
  
  bool animateValues;

  bool showIndicators;
} Settings;

bool LoadSensorConfig(const char* fileName, size_t* count, sensorData **dt);

bool LoadSettings(const char* fileName, Settings* s);

bool SaveSettings(const char* fileName, const Settings* s);

#endif
