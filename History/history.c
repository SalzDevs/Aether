#include "history.h"
#include <stdio.h>
#include <stdlib.h>

void initSensorHistory(sensorHistory* h, size_t capacity) {
  if (capacity == 0) capacity = 1;
  h->capacity = capacity;
  h->size = 0;
  h->head = 0;
  h->data = (sensorData*)malloc(capacity * sizeof(sensorData));
}

void destroySensorHistory(sensorHistory* h) {
  if (h == NULL) return;
  free(h->data);
  h->data = NULL;
  h->capacity = 0;
  h->size = 0;
  h->head = 0;
}

size_t sensorHistorySize(const sensorHistory* h) {
  return h->size;
}

const sensorData* sensorHistoryAt(const sensorHistory* h, size_t index) {
  if (h->size == 0 || index >= h->size) return NULL;
  // oldest lives at (head - size) mod capacity
  size_t oldest = (h->head + h->capacity - h->size) % h->capacity;
  size_t slot = (oldest + index) % h->capacity;
  return &h->data[slot];
}

void pushSensorReading(sensorHistory* h, sensorData d) {
  h->data[h->head] = d;
  h->head = (h->head + 1) % h->capacity;
  if (h->size < h->capacity) h->size++;
}
