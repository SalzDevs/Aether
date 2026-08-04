#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "scheduler/scheduler.h"
#include "sensor/sensor.h"


int main() {
  sensor s;
  initSensor(&s);
  printSensorData(s);
  printf("\nAfter Updating Data on sensor\n");
  sensorDataUpdate(&s);
  printSensorData(s);
  
  Task t;
  initTask(&t, 30);
  printTask(t); 
  return 0;
}
