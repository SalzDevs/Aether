#include "sensor.h"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void initSensor(sensor* s){
  s->airSpeed = 0.0;
  s->altitude = 0.0;
  s->engineTemperature = 0;
  s->fuelLevel = 0;
  s->baterryVoltage = 0;
}

void printSensorData(sensor s) {
  printf("Air Speed:%f\n",s.airSpeed);
  printf("Altitude:%f\n",s.altitude);
  printf("Engine Temperature:%d\n",s.engineTemperature);
  printf("Fuel Level:%d\n",s.fuelLevel);
  printf("Baterry Voltage:%d\n",s.baterryVoltage);
}

int sensorToString(sensor s, char* buf, size_t bufSize) {
  return snprintf(buf,bufSize,"Air Speed:%f Altitude:%f Engine Temp:%d Fuel:%d Battery:%d\n",
    s.airSpeed, s.altitude, s.engineTemperature, s.fuelLevel, s.baterryVoltage);
}

void sensorDataUpdate(sensor* s) {
  s->airSpeed = rand(); 
  s->altitude = rand();
  s->engineTemperature = rand();
  s->fuelLevel = rand();
  s->baterryVoltage = rand();
}


