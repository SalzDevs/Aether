#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>

void initSensor(sensor* s, int id) {
  s->id = id;
}

void initSensorData(sensorData* d) {
  d->sensorId = 0;
  d->airSpeed = 0.0;
  d->altitude = 0.0;
  d->engineTemperature = 0;
  d->fuelLevel = 0;
  d->batteryVoltage = 0;
}

void printSensorData(sensorData d) {
  printf("Sensor Id:%d\n", d.sensorId);
  printf("Air Speed:%f\n", d.airSpeed);
  printf("Altitude:%f\n", d.altitude);
  printf("Engine Temperature:%d\n", d.engineTemperature);
  printf("Fuel Level:%d\n", d.fuelLevel);
  printf("Battery Voltage:%d\n", d.batteryVoltage);
}

int sensorDataToString(sensorData d, char* buf, size_t bufSize) {
  return snprintf(buf, bufSize, "Sensor %d | Air Speed:%f Altitude:%f Engine Temp:%d Fuel:%d Battery:%d\n",
    d.sensorId, d.airSpeed, d.altitude, d.engineTemperature, d.fuelLevel, d.batteryVoltage);
}

void sensorDataUpdate(sensorData* d) {
  d->airSpeed = rand();
  d->altitude = rand();
  d->engineTemperature = rand();
  d->fuelLevel = rand();
  d->batteryVoltage = rand();
}