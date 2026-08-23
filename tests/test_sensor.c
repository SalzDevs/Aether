#include "../sensor/sensor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Builds a sensor with a few generic metrics for simulation tests.
static sensorData make_default_sensor(void) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "temperature", "C", 20.0f);
  addMetric(&d, "humidity", "%", 50.0f);
  return d;
}

static void sensor_init_test() {
  sensor s;
  initSensor(&s, 42);
  assert(s.id == 42);
}

static void sensor_data_init_test() {
  sensorData d;
  initSensorData(&d);
  assert(d.id == 0);
  assert(d.metricCount == 0);
  assert(d.lastUpdate_s == 0.0f);
}

static void test_add_metric_stores_name_unit_and_value(void) {
  sensorData d;
  initSensorData(&d);

  assert(addMetric(&d, "temperature", "C", 22.5f));
  assert(d.metricCount == 1);

  Metric* m = findMetric(&d, "temperature");
  assert(m != NULL);
  assert(strcmp(m->name, "temperature") == 0);
  assert(strcmp(m->unit, "C") == 0);
  assert(m->value == 22.5f);
  assert(m->initialValue == 22.5f);
}

static void test_add_metric_rejects_full_sensor(void) {
  sensorData d;
  initSensorData(&d);

  char name[8];
  for (int i = 0; i < MAX_METRICS; i++) {
    snprintf(name, sizeof(name), "m%d", i);
    assert(addMetric(&d, name, "", 1.0f));
  }
  assert(!addMetric(&d, "overflow", "", 1.0f));
  assert(d.metricCount == MAX_METRICS);
}

static void test_add_metric_rejects_empty_name(void) {
  sensorData d;
  initSensorData(&d);
  assert(!addMetric(&d, "", "", 1.0f));
  assert(!addMetric(&d, NULL, "", 1.0f));
  assert(d.metricCount == 0);
}

static void test_find_metric_missing_returns_null(void) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "a", "", 1.0f);
  assert(findMetric(&d, "a") != NULL);
  assert(findMetric(&d, "b") == NULL);
}

static void test_update_changes_values(void) {
  sensorData d = make_default_sensor();

  srand(42);
  sensorDataUpdate(&d, 0.0f);
  float temp1 = findMetric(&d, "temperature")->value;

  sensorDataUpdate(&d, 1.0f);
  assert(findMetric(&d, "temperature")->value != temp1);
}

static void test_update_is_deterministic_with_seed(void) {
  sensorData d1 = make_default_sensor();
  sensorData d2 = make_default_sensor();

  srand(42);
  sensorDataUpdate(&d1, 5.0f);

  srand(42);
  sensorDataUpdate(&d2, 5.0f);

  for (size_t i = 0; i < d1.metricCount; i++) {
    assert(d1.metrics[i].value == d2.metrics[i].value);
  }
}

static void test_update_stays_near_initial_value(void) {
  // A single walk step moves a metric by at most RANDOM_WALK_FRACTION
  // of its initial value.
  const float FRACTION = 0.05f;
  sensorData d = make_default_sensor();

  srand(7);
  sensorDataUpdate(&d, 0.0f);

  Metric* m = findMetric(&d, "temperature");
  float maxDelta = 20.0f * FRACTION + 0.001f;
  assert(m->value > 20.0f - maxDelta && m->value < 20.0f + maxDelta);
}

static void test_update_never_negative(void) {
  sensorData d = make_default_sensor();

  srand(1234);
  for (int i = 0; i < 100; i++) {
    sensorDataUpdate(&d, (float)i);
    assert(findMetric(&d, "temperature")->value >= 0.0f);
    assert(findMetric(&d, "humidity")->value >= 0.0f);
  }
}

static void test_zero_initial_value_metric_stays_constant(void) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "offset", "", 0.0f);

  srand(99);
  sensorDataUpdate(&d, 10.0f);
  assert(findMetric(&d, "offset")->value == 0.0f);
}

static void test_to_string_contains_sensor_id_and_metrics(void) {
  sensorData d = make_default_sensor();
  d.id = 7;

  char buf[256];
  sensorDataToString(d, buf, sizeof(buf));

  assert(strstr(buf, "Sensor 7") != NULL);
  assert(strstr(buf, "temperature(C):20") != NULL);
  assert(strstr(buf, "humidity(%):50") != NULL);
}

static void test_to_string_truncates_safely(void) {
  sensorData d = make_default_sensor();

  char tiny[10];
  int len = sensorDataToString(d, tiny, sizeof(tiny));
  assert(len >= 0);
  assert(strlen(tiny) < sizeof(tiny)); // always null-terminated within bounds
}

int main() {
  sensor_init_test();
  sensor_data_init_test();
  test_add_metric_stores_name_unit_and_value();
  test_add_metric_rejects_full_sensor();
  test_add_metric_rejects_empty_name();
  test_find_metric_missing_returns_null();
  test_update_changes_values();
  test_update_is_deterministic_with_seed();
  test_update_stays_near_initial_value();
  test_update_never_negative();
  test_zero_initial_value_metric_stays_constant();
  test_to_string_contains_sensor_id_and_metrics();
  test_to_string_truncates_safely();
  printf("All sensor tests passed\n");
  return 0;
}
