#ifndef QEUE_H
#define QEUE_H

#include "../sensor/sensor.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  size_t allocated_size;
  size_t current_size;
  sensorData* data;
} sensorQeue;


void initSensorQeue(sensorQeue* sq); 

bool isSensorQeueEmpty(sensorQeue* sq); 

void addElemToQueue(sensorQeue *sq, sensorData d);

sensorData removeElemFromQeue(sensorQeue *sq);

void printQeue(sensorQeue* sq);

#endif