#ifndef QUEUE_H
#define QUEUE_H

#include "../sensor/sensor.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  size_t allocated_size;
  size_t current_size;
  sensorData* data;
} sensorQueue;


void initSensorQueue(sensorQueue* sq);

void destroySensorQueue(sensorQueue* sq);

bool isSensorQueueEmpty(sensorQueue* sq);

void addElemToQueue(sensorQueue *sq, sensorData d);

sensorData removeElemFromQueue(sensorQueue *sq);

void printQueue(sensorQueue* sq);

#endif
