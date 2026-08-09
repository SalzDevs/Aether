#ifndef QEUE_H
#define QEUE_H

#include "../sensor/sensor.h"
#include <stddef.h>

typedef struct {
  size_t allocated_size;
  size_t current_size;
  sensor* data;
} sensorQeue;


void initSensorQeue(sensorQeue* sq); 


void addElemToQueue(sensorQeue *sq, sensor s);

sensor removeElemFromQeue(sensorQeue *sq);

void printQeue(sensorQeue* sq);

#endif
