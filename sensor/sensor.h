#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>

typedef struct {
  float airSpeed;
  float altitude;
  int engineTemperature;
  int fuelLevel;
  int baterryVoltage;
} sensor;

void initSensor(sensor* s);

void printSensorData(sensor s); 

int sensorToString(sensor s, char* buf, size_t bufSize);

void sensorDataUpdate(sensor* s);


#endif
