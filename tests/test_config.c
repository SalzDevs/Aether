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

static void test_many_sensors_grow_registry(void) {
  // The old parser silently dropped sensors past a hard cap (32).
  // Generate 40 and make sure every one of them is loaded.
  const char* path = "/tmp/aether_test_many.yaml";
  FILE* f = fopen(path, "w");
  assert(f != NULL);
  fprintf(f, "sensors:\n");
  for (int i = 1; i <= 40; i++) {
    fprintf(f,
        "  - id: %d\n"
        "    name: sensor_%d\n"
        "    metrics:\n"
        "      - name: value\n"
        "        unit: u\n"
        "        value: %d\n",
        i, i, i * 10);
  }
  fclose(f);

  size_t count = 0;
  sensorData* sensors = NULL;
  bool ok = LoadSensorConfig(path, &count, &sensors);
  assert(ok);
  assert(count == 40);
  assert(sensors[0].id == 1);
  assert(value_of(&sensors[0], "value") == 10.0f);
  assert(sensors[39].id == 40);
  assert(value_of(&sensors[39], "value") == 400.0f);

  free(sensors);
  remove(path);
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
  test_many_sensors_grow_registry();
  test_file_to_sensors_missing_file();
  printf("All config tests passed\n");
  return 0;
}
