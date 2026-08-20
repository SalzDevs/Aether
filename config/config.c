#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SENSORS 32

static configStatus readConfigFile(const char* fileName, char** buf_out) {
  FILE* fptr = fopen(fileName, "r");
  if (fptr == NULL) return CONFIG_ERROR;

  fseek(fptr, 0L, SEEK_END);
  long numbytes = ftell(fptr);
  rewind(fptr);
  if (numbytes < 0) { fclose(fptr); return CONFIG_ERROR; }

  char* buffer = (char*)calloc((size_t)numbytes + 1, 1);
  if (buffer == NULL) {
    fclose(fptr); return CONFIG_ERROR;
  }

  size_t read = fread(buffer, 1, (size_t)numbytes, fptr);
  fclose(fptr);
  if (read != (size_t)numbytes) {
    free(buffer); return CONFIG_ERROR;
  }

  *buf_out = buffer;
  return CONFIG_OK;
}

static sensorData* fileToSensors(const char* fileName, size_t* count) {
  char* contents;
  
  if (readConfigFile(fileName, &contents) != CONFIG_OK) {
    return NULL;
  }

  sensorData* sensors = malloc(MAX_SENSORS * sizeof(sensorData));
  if (sensors == NULL) { free(contents); return NULL; }
  *count = 0;

  char* p = contents;
  // skip "sensors:" header line
  char* nl = strchr(p, '\n');
  if (nl != NULL) p = nl + 1;

  int consumed;
while (*count < MAX_SENSORS &&
       sscanf(
           p,
           " - id: %d name: %*s data: "
           "airSpeed: %f altitude: %f engineTemperature: %d "
           "fuelLevel: %d batteryVoltage: %d%n",
           &sensors[*count].sensorId,
           &sensors[*count].airSpeed,
           &sensors[*count].altitude,
           &sensors[*count].engineTemperature,
           &sensors[*count].fuelLevel,
           &sensors[*count].batteryVoltage,
           &consumed
       ) == 6) {
    (*count)++;
    p += consumed;
  }

  free(contents);
  
  if (*count == 0) {
    free(sensors);
    return NULL;
  }
  
  return sensors;
}

bool LoadSensorConfig(const char *fileName, size_t *count, sensorData **out) {
  sensorData *dt = fileToSensors(fileName, count);
  if (dt == NULL) return false;
  *out = dt;
  return true;
}
