#include "../sensor/sensor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Builds a sensor carrying the five legacy metrics so simulation
// behavior can be tested by name.
static sensorData make_default_sensor(void) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "airSpeed", "m/s", 120.5f);
  addMetric(&d, "altitude", "m", 1000.0f);
  addMetric(&d, "engineTemperature", "C", 85.0f);
  addMetric(&d, "fuelLevel", "%", 100.0f);
  addMetric(&d, "batteryVoltage", "V", 12.0f);
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

  assert(addMetric(&d, "fuelLevel", "%", 60.0f));
  assert(d.metricCount == 1);

  Metric* m = findMetric(&d, "fuelLevel");
  assert(m != NULL);
  assert(strcmp(m->name, "fuelLevel") == 0);
  assert(strcmp(m->unit, "%") == 0);
  assert(m->value == 60.0f);
  assert(m->initialValue == 60.0f);
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

static void test_update_fields_in_range(void) {
  sensorData d = make_default_sensor();

  sensorDataUpdate(&d, 0.0f);

  Metric* m;
  assert((m = findMetric(&d, "airSpeed")) != NULL);
  assert(m->value >= 0 && m->value <= (float)RAND_MAX);
  assert((m = findMetric(&d, "altitude")) != NULL);
  assert(m->value >= 0 && m->value <= (float)RAND_MAX);
  assert((m = findMetric(&d, "batteryVoltage")) != NULL);
  assert(m->value >= 0 && m->value <= (float)RAND_MAX);
  assert((m = findMetric(&d, "fuelLevel")) != NULL);
  assert(m->value >= 0 && m->value <= (float)RAND_MAX);
}

static void test_consecutive_updates_differ(void) {
  sensorData d = make_default_sensor();
  sensorDataUpdate(&d, 0.0f);
  float speed1 = findMetric(&d, "airSpeed")->value;
  sensorDataUpdate(&d, 0.0f);
  assert(findMetric(&d, "airSpeed")->value != speed1);
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

static void test_fuel_depletes_over_time(void) {
  sensorData d = make_default_sensor(); // fuel initialValue = 100

  sensorDataUpdate(&d, 10.0f);
  assert(findMetric(&d, "fuelLevel")->value == 90.0f);

  sensorDataUpdate(&d, 20.0f);
  assert(findMetric(&d, "fuelLevel")->value == 80.0f);
}

static void test_fuel_never_goes_negative(void) {
  sensorData d = make_default_sensor();

  sensorDataUpdate(&d, 1000.0f);
  assert(findMetric(&d, "fuelLevel")->value == 0.0f);
}

// Thermal model constants mirrored from sensor.c
#define AMBIENT_TEMP_C   25.0f
#define HEAT_GENERATED_W 1500.0f
#define ENGINE_MASS_KG   120.0f
#define SPECIFIC_HEAT    450.0f
#define COOLING_RATE     0.02f

static float expectedEquilibriumTemp(void) {
  return AMBIENT_TEMP_C +
         HEAT_GENERATED_W / (COOLING_RATE * ENGINE_MASS_KG * SPECIFIC_HEAT);
}

static void test_engine_temp_cools_when_above_equilibrium(void) {
  sensorData d = make_default_sensor(); // engineTemperature starts at 85 C

  sensorDataUpdate(&d, 3.0f);
  Metric* m = findMetric(&d, "engineTemperature");
  assert(m->value < 85.0f);
  assert(m->value >= expectedEquilibriumTemp());
}

static void test_engine_temp_heats_when_below_equilibrium(void) {
  sensorData d = make_default_sensor();
  findMetric(&d, "engineTemperature")->value = AMBIENT_TEMP_C;

  sensorDataUpdate(&d, 10.0f);
  assert(findMetric(&d, "engineTemperature")->value > AMBIENT_TEMP_C);
}

static void test_engine_temp_never_negative(void) {
  sensorData d = make_default_sensor();
  findMetric(&d, "engineTemperature")->value = -50.0f;

  sensorDataUpdate(&d, 100.0f);
  assert(findMetric(&d, "engineTemperature")->value >= 0.0f);
}

static void test_engine_temp_zero_time_step_is_noop(void) {
  sensorData d = make_default_sensor();

  sensorDataUpdate(&d, 5.0f);
  float temp = findMetric(&d, "engineTemperature")->value;
  sensorDataUpdate(&d, 5.0f); // same timestamp -> dt = 0 -> unchanged
  assert(findMetric(&d, "engineTemperature")->value == temp);
}

static void test_to_string_contains_sensor_id_and_metrics(void) {
  sensorData d = make_default_sensor();
  d.id = 7;

  char buf[256];
  sensorDataToString(d, buf, sizeof(buf));

  assert(strstr(buf, "Sensor 7") != NULL);
  assert(strstr(buf, "airSpeed(m/s):120.5") != NULL);
  assert(strstr(buf, "fuelLevel(%):100") != NULL);
  assert(strstr(buf, "batteryVoltage(V):12") != NULL);
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
  test_update_fields_in_range();
  test_consecutive_updates_differ();
  test_update_is_deterministic_with_seed();
  test_fuel_depletes_over_time();
  test_fuel_never_goes_negative();
  test_engine_temp_cools_when_above_equilibrium();
  test_engine_temp_heats_when_below_equilibrium();
  test_engine_temp_never_negative();
  test_engine_temp_zero_time_step_is_noop();
  test_to_string_contains_sensor_id_and_metrics();
  test_to_string_truncates_safely();
  printf("All sensor tests passed\n");
  return 0;
}
