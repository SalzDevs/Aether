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

bool sensorHistoryMetricStats(const sensorHistory* h, size_t metricIdx,
                              float* outMin, float* outMax, float* outAvg) {
  size_t count = sensorHistorySize(h);
  if (count == 0) return false;

  float min = 0.0f, max = 0.0f, sum = 0.0f;
  for (size_t i = 0; i < count; i++) {
    const sensorData* reading = sensorHistoryAt(h, i);
    if (metricIdx >= reading->metricCount) return false;
    float v = reading->metrics[metricIdx].value;
    if (i == 0 || v < min) min = v;
    if (i == 0 || v > max) max = v;
    sum += v;
  }
  *outMin = min;
  *outMax = max;
  *outAvg = sum / (float)count;
  return true;
}

void pushSensorReading(sensorHistory* h, sensorData d) {
  h->data[h->head] = d;
  h->head = (h->head + 1) % h->capacity;
  if (h->size < h->capacity) h->size++;
}
