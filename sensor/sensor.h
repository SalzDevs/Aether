#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>

typedef struct {
  int id;
} sensor;

typedef struct {
  int sensorId;
  float airSpeed;
  float altitude;
  int engineTemperature;
  int fuelLevel;
  int batteryVoltage;
} sensorData;

void initSensor(sensor* s, int id);

void initSensorData(sensorData* d);

void printSensorData(sensorData d);

int sensorDataToString(sensorData d, char* buf, size_t bufSize);

void sensorDataUpdate(sensorData* d);

#endif
