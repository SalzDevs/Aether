#ifndef HISTORY_H
#define HISTORY_H

#include "../sensor/sensor.h"
#include <stddef.h>

// Bounded per-sensor reading history (ring buffer).
// Pushing into a full history drops the oldest reading.
typedef struct {
  size_t capacity;     // max readings kept
  size_t size;         // valid readings (<= capacity)
  size_t head;         // next write position
  sensorData* data;    // capacity entries
} sensorHistory;

// Allocates storage for `capacity` readings. Capacity must be > 0.
void initSensorHistory(sensorHistory* h, size_t capacity);

void destroySensorHistory(sensorHistory* h);

// Number of valid readings currently stored (<= capacity).
size_t sensorHistorySize(const sensorHistory* h);

// Reading `index` counted from the OLDEST (0) to the newest (size-1).
// Returns NULL when the index is out of range.
const sensorData* sensorHistoryAt(const sensorHistory* h, size_t index);

// Adds a reading; when the history is full, the oldest is dropped.
void pushSensorReading(sensorHistory* h, sensorData d);

#endif
