#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "scheduler/scheduler.h"
#include "sensor/sensor.h"

typedef struct {
  size_t intial_size;
  size_t current_size;
  sensor* data;
} sensorQeue;


void initSensorQeue(sensorQeue* sq) {
  sq->intial_size = 100;
  sq->current_size = 0;
  sq->data = (sensor*)malloc(sizeof(sensor)); 
}


int main() {
  sensorQeue sq;
  initSensorQeue(&sq);
  printf("sensorQeue data after init: {%zu} {%zu} {%p}\n",sq.intial_size, sq.current_size, sq.data);
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
