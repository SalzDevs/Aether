#include "../config/config.h"
#include "../sensor/sensor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Returns a metric's value, failing the test if the metric is missing.
static float value_of(sensorData* s, const char* name) {
  Metric* m = findMetric(s, name);
  assert(m != NULL);
  return m->value;
}

static void test_file_to_sensors(void) {
  size_t count = 0;
  sensorData* sensors = NULL;
  bool ok = LoadSensorConfig("config/sensors.yaml", &count, &sensors);
  assert(ok);

  assert(sensors != NULL);
  assert(count == 5);

  assert(sensors[0].id == 1);
  assert(strcmp(sensors[0].name, "temperature_sensor") == 0);
  assert(value_of(&sensors[0], "temperature") == 22.5f);
  assert(value_of(&sensors[0], "humidity") == 45.0f);

  // Metrics carry units and remember their initial value
  Metric* humidity = findMetric(&sensors[0], "humidity");
  assert(strcmp(humidity->unit, "%") == 0);
  assert(humidity->initialValue == 45.0f);

  Metric* pressure = findMetric(&sensors[1], "pressure");
  assert(pressure != NULL);
  assert(pressure->value == 1013.25f);
  assert(strcmp(pressure->unit, "hPa") == 0);

  assert(sensors[2].id == 3);
  assert(value_of(&sensors[2], "voltage") == 12.0f);
  assert(value_of(&sensors[2], "current") == 3.5f);

  assert(sensors[4].id == 5);
  assert(value_of(&sensors[4], "acceleration") == 9.81f);
  assert(value_of(&sensors[4], "frequency") == 50.0f);

  free(sensors);
}

static void test_file_to_sensors_missing_file(void) {
  size_t count = 0;
  sensorData* sensors = NULL;
  bool ok = LoadSensorConfig("config/does_not_exist.yaml", &count, &sensors);

  assert(!ok);
  assert(sensors == NULL);
  assert(count == 0);
}

int main() {
  test_file_to_sensors();
  test_file_to_sensors_missing_file();
  printf("All config tests passed\n");
  return 0;
}
