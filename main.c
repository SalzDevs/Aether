#include "stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  float airSpeed;
  float altitude;
  int engineTemperature;
  int fuelLevel;
  int baterryVoltage;
} sensor;

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

void sensorDataUpdate(sensor* s) {
  srand(time(NULL));

  s->airSpeed = rand(); 
  s->altitude = rand();
  s->engineTemperature = rand();
  s->fuelLevel = rand();
  s->baterryVoltage = rand();
}

int main() {
  sensor s;
  initSensor(&s);
  printSensorData(s);
  printf("\nAfter Updating Data on sensor\n");
  sensorDataUpdate(&s);
  printSensorData(s);
  return 0;
}
