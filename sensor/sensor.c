#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Each update moves a metric by up to +/- this fraction of its
// initial value. Metrics with an initial value of 0 stay constant.
#define RANDOM_WALK_FRACTION 0.05f

void initSensor(sensor* s, int id) {
  s->id = id;
}

void initSensorData(sensorData* d) {
  memset(d, 0, sizeof(*d));
}

bool addMetric(sensorData* d, const char* name, const char* unit, float value) {
  if (d == NULL || name == NULL || name[0] == '\0') return false;
  if (d->metricCount >= MAX_METRICS) return false;

  Metric* m = &d->metrics[d->metricCount];
  snprintf(m->name, sizeof(m->name), "%s", name);
  snprintf(m->unit, sizeof(m->unit), "%s", unit != NULL ? unit : "");
  m->value = value;
  m->initialValue = value;
  d->metricCount++;
  return true;
}

Metric* findMetric(sensorData* d, const char* name) {
  if (d == NULL || name == NULL) return NULL;
  for (size_t i = 0; i < d->metricCount; i++) {
    if (strcmp(d->metrics[i].name, name) == 0) return &d->metrics[i];
  }
  return NULL;
}

void printSensorData(sensorData d) {
  printf("Sensor Id:%d Name:%s\n", d.id, d.name);
  for (size_t i = 0; i < d.metricCount; i++) {
    Metric* m = &d.metrics[i];
    printf("  %s (%s): %g\n", m->name, m->unit, m->value);
  }
}

int sensorDataToString(sensorData d, char* buf, size_t bufSize) {
  if (buf == NULL || bufSize == 0) return 0;

  size_t used = (size_t)snprintf(buf, bufSize, "Sensor %d |", d.id);

  for (size_t i = 0; i < d.metricCount && used < bufSize - 1; i++) {
    Metric* m = &d.metrics[i];
    used += (size_t)snprintf(buf + used, bufSize - used, " %s(%s):%g",
                             m->name, m->unit, m->value);
  }

  return (int)used;
}

static float randomWalk(float value, float maxDelta) {
  float delta = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * maxDelta;
  float next = value + delta;
  return next > 0.0f ? next : 0.0f;
}

// Placeholder simulation: nudges every metric around its initial value.
// Replace with real data sources later.
void sensorDataUpdate(sensorData* d, float time_s) {
  d->lastUpdate_s = time_s;

  for (size_t i = 0; i < d->metricCount; i++) {
    Metric* m = &d->metrics[i];
    float initialValue = m->initialValue < 0.0f ? -m->initialValue : m->initialValue;
    float maxDelta = initialValue * RANDOM_WALK_FRACTION;
    if (maxDelta <= 0.0f) continue;
    m->value = randomWalk(m->value, maxDelta);
  }
}
