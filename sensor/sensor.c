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
  d->initialFuelLevel = 0;
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
#define FUEL_BURN_RATE 1

static int updateFuelLevel(int initialFuelLevel, int burnRate, float time_s) {
  int remaining_fuel = initialFuelLevel - (int)(burnRate * time_s);
  return remaining_fuel > 0 ? remaining_fuel : 0;
}

void sensorDataUpdate(sensorData* d, float time_s) {
  d->airSpeed = rand();
  d->altitude = rand();
  d->engineTemperature = rand();
  d->fuelLevel = updateFuelLevel(d->initialFuelLevel, FUEL_BURN_RATE, time_s);
  d->batteryVoltage = rand();
}
