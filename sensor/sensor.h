#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stddef.h>

#define SENSOR_NAME_MAX 32
#define METRIC_NAME_MAX 32
#define METRIC_UNIT_MAX 16
#define MAX_METRICS 8

// A minimal sensor identity
typedef struct {
  int id;
} sensor;

// A single named measurement of a sensor, e.g. ("fuel", "%", 42).
typedef struct {
  char name[METRIC_NAME_MAX];
  char unit[METRIC_UNIT_MAX];
  float value;
  // Value loaded from config; simulation models may reference it
  float initialValue;
} Metric;

// A sensor is an identity plus an open-ended list of metrics.
// Nothing outside the simulation knows which metrics exist.
typedef struct {
  int id;
  char name[SENSOR_NAME_MAX];
  Metric metrics[MAX_METRICS];
  size_t metricCount;
  // Timestamp of the last sensorDataUpdate call
  float lastUpdate_s;
} sensorData;

void initSensor(sensor* s, int id);

void initSensorData(sensorData* d);

// Adds a metric with the given name, unit and initial value.
// The metric's value starts at initialValue.
// Returns false if the sensor is full or the name is missing/empty.
bool addMetric(sensorData* d, const char* name, const char* unit, float value);

// Returns the metric with the given name, or NULL if not present.
Metric* findMetric(sensorData* d, const char* name);

void printSensorData(sensorData d);

int sensorDataToString(sensorData d, char* buf, size_t bufSize);

void sensorDataUpdate(sensorData* d, float time_s);

#endif
