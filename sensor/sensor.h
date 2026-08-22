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
  int initialFuelLevel;
  int batteryVoltage;
  // simulation state
  float engineTemp;    // current simulated engine temperature in Celsius (float state)
  float lastUpdate_s;  // timestamp of the last sensorDataUpdate call
} sensorData;

void initSensor(sensor* s, int id);

void initSensorData(sensorData* d);

void printSensorData(sensorData d);

int sensorDataToString(sensorData d, char* buf, size_t bufSize);

void sensorDataUpdate(sensorData* d, float time_s);

#endif
