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

// Writes a YAML string to a temp file and loads it.
static bool loadYamlString(const char* content, size_t* count, sensorData** out) {
  const char* path = "/tmp/aether_stress.yaml";
  FILE* f = fopen(path, "w");
  assert(f != NULL);
  fputs(content, f);
  fclose(f);
  return LoadSensorConfig(path, count, out);
}

static void test_stress_hundred_sensors(void) {
  char yaml[16384];
  int written = snprintf(yaml, sizeof(yaml), "sensors:\n");
  for (int i = 1; i <= 100; i++) {
    written += snprintf(yaml + written, sizeof(yaml) - (size_t)written,
        "  - id: %d\n"
        "    name: s%d\n"
        "    metrics:\n"
        "      - name: v\n"
        "        unit: u\n"
        "        value: %d\n", i, i, i);
  }
  assert(written > 0 && (size_t)written < sizeof(yaml));

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(loadYamlString(yaml, &count, &sensors));
  assert(count == 100);
  assert(sensors[0].id == 1);
  assert(sensors[99].id == 100);
  free(sensors);
}

static void test_stress_sensors_without_id_are_skipped(void) {
  const char* yaml =
      "sensors:\n"
      "  - name: no_id_here\n"
      "    metrics:\n"
      "      - name: v\n"
      "        unit: u\n"
        "        value: 1\n"
      "  - id: 7\n"
      "    name: valid\n"
      "    metrics:\n"
      "      - name: v\n"
      "        unit: u\n"
      "        value: 2\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(loadYamlString(yaml, &count, &sensors));
  assert(count == 1); // the id-less entry is consumed and dropped
  assert(sensors[0].id == 7);
  free(sensors);
}

static void test_stress_sensor_without_metrics_is_accepted(void) {
  const char* yaml =
      "sensors:\n"
      "  - id: 1\n"
      "    name: bare_sensor\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(loadYamlString(yaml, &count, &sensors));
  assert(count == 1);
  assert(sensors[0].metricCount == 0);
  free(sensors);
}

static void test_stress_long_names_are_truncated_safely(void) {
  const char* yaml =
      "sensors:\n"
      "  - id: 1\n"
      "    name: a_very_long_sensor_name_well_past_the_limit_for_testing\n"
      "    metrics:\n"
      "      - name: v\n"
      "        unit: u\n"
      "        value: 1\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(loadYamlString(yaml, &count, &sensors));
  assert(count == 1);
  assert(strlen(sensors[0].name) == SENSOR_NAME_MAX - 1); // truncated, not overflowed
  free(sensors);
}

static void test_stress_unknown_keys_are_ignored(void) {
  const char* yaml =
      "version: 2\n"
      "sensors:\n"
      "  - id: 1\n"
      "    name: weird\n"
      "    location: basement\n"
      "    tags: [a, b]\n"
      "    metrics:\n"
      "      - name: v\n"
      "        unit: u\n"
      "        value: 1\n"
      "        calibration: 0.98\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(loadYamlString(yaml, &count, &sensors));
  assert(count == 1);
  assert(value_of(&sensors[0], "v") == 1.0f);
  assert(findMetric(&sensors[0], "calibration") == NULL);
  free(sensors);
}

static void test_stress_malformed_yaml_fails_cleanly(void) {
  const char* yaml =
      "sensors:\n"
      "  - id: %%%not valid yaml at all[[[\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(!loadYamlString(yaml, &count, &sensors));
  assert(sensors == NULL);
  assert(count == 0);
}

static void test_stress_header_only_yields_no_sensors(void) {
  const char* yaml = "sensors:\n";

  size_t count = 0;
  sensorData* sensors = NULL;
  assert(!loadYamlString(yaml, &count, &sensors));
  assert(sensors == NULL);
  assert(count == 0);
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
  test_stress_hundred_sensors();
  test_stress_sensors_without_id_are_skipped();
  test_stress_sensor_without_metrics_is_accepted();
  test_stress_long_names_are_truncated_safely();
  test_stress_unknown_keys_are_ignored();
  test_stress_malformed_yaml_fails_cleanly();
  test_stress_header_only_yields_no_sensors();
  test_file_to_sensors_missing_file();
  printf("All config tests passed\n");
  return 0;
}
