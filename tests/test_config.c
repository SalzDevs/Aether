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
  assert(value_of(&sensors[0], "airSpeed") == 120.5f);
  assert(value_of(&sensors[0], "altitude") == 1000.0f);
  assert(value_of(&sensors[0], "engineTemperature") == 85.0f);
  assert(value_of(&sensors[0], "fuelLevel") == 60.0f);
  assert(value_of(&sensors[0], "batteryVoltage") == 12.0f);

  // Metrics carry units and remember their initial value
  Metric* fuel = findMetric(&sensors[0], "fuelLevel");
  assert(strcmp(fuel->unit, "%") == 0);
  assert(fuel->initialValue == 60.0f);
  Metric* temp = findMetric(&sensors[0], "engineTemperature");
  assert(strcmp(temp->unit, "C") == 0);

  // Sensor names are parsed too
  assert(strcmp(sensors[0].name, "airspeed_sensor") == 0);
  assert(strcmp(sensors[2].name, "engine_sensor") == 0);

  assert(sensors[3].id == 4);
  assert(value_of(&sensors[3], "fuelLevel") == 42.0f);

  assert(sensors[4].id == 5);
  assert(value_of(&sensors[4], "airSpeed") == 115.8f);
  assert(value_of(&sensors[4], "altitude") == 1100.0f);
  assert(value_of(&sensors[4], "engineTemperature") == 87.0f);
  assert(value_of(&sensors[4], "fuelLevel") == 42.0f);
  assert(value_of(&sensors[4], "batteryVoltage") == 12.0f);

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
