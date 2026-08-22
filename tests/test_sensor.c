#include "../sensor/sensor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void sensor_init_test() {
  sensor s;
  initSensor(&s, 42);
  assert(s.id == 42);
}

static void sensor_data_init_test() {
  sensorData d;
  initSensorData(&d);
  assert(d.sensorId == 0);
  assert(d.airSpeed == 0.0);
  assert(d.altitude == 0.0);
  assert(d.engineTemperature == 0);
  assert(d.fuelLevel == 0);
  assert(d.initialFuelLevel == 0);
  assert(d.batteryVoltage == 0);
  assert(d.engineTemp == 0.0f);
  assert(d.lastUpdate_s == 0.0f);
}

static void test_update_fields_in_range(void) {
  sensorData d;
  initSensorData(&d);
  sensorDataUpdate(&d, 0.0f);
  assert(d.airSpeed >= 0 && d.airSpeed <= (float)RAND_MAX);
  assert(d.altitude >= 0 && d.altitude <= (float)RAND_MAX);
  assert(d.engineTemperature >= 0 && d.engineTemperature <= RAND_MAX);
  assert(d.fuelLevel >= 0 && d.fuelLevel <= RAND_MAX);
  assert(d.batteryVoltage >= 0 && d.batteryVoltage <= RAND_MAX);
}

static void test_consecutive_updates_differ(void) {
  sensorData d;
  initSensorData(&d);
  sensorDataUpdate(&d, 0.0f);
  float speed1 = d.airSpeed;
  sensorDataUpdate(&d, 0.0f);
  assert(d.airSpeed != speed1);
}

static void test_update_is_deterministic_with_seed(void) {
  sensorData d1, d2;
  initSensorData(&d1);
  initSensorData(&d2);

  srand(42);
  sensorDataUpdate(&d1, 5.0f);

  srand(42);
  sensorDataUpdate(&d2, 5.0f);

  assert(d1.airSpeed == d2.airSpeed);
  assert(d1.altitude == d2.altitude);
  assert(d1.engineTemperature == d2.engineTemperature);
  assert(d1.fuelLevel == d2.fuelLevel);
  assert(d1.batteryVoltage == d2.batteryVoltage);
}

static void test_fuel_depletes_over_time(void) {
  sensorData d;
  initSensorData(&d);
  d.initialFuelLevel = 100;

  sensorDataUpdate(&d, 10.0f);
  assert(d.fuelLevel == 90);

  sensorDataUpdate(&d, 20.0f);
  assert(d.fuelLevel == 80);
}

static void test_fuel_never_goes_negative(void) {
  sensorData d;
  initSensorData(&d);
  d.initialFuelLevel = 100;

  sensorDataUpdate(&d, 1000.0f);
  assert(d.fuelLevel == 0);
}

static void test_to_string_contains_sensor_id(void) {
  sensorData d;
  initSensorData(&d);
  d.sensorId = 7;
  char buf[256];
  sensorDataToString(d, buf, sizeof(buf));
  assert(strstr(buf, "Sensor 7") != NULL);
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
  sensorData d;
  initSensorData(&d);
  d.engineTemp = 85.0f; // config starting temperature

  sensorDataUpdate(&d, 3.0f);
  assert(d.engineTemperature < 85);
  assert(d.engineTemperature >= (int)expectedEquilibriumTemp());
}

static void test_engine_temp_heats_when_below_equilibrium(void) {
  sensorData d;
  initSensorData(&d);
  d.engineTemp = AMBIENT_TEMP_C;

  float before = d.engineTemperature;
  sensorDataUpdate(&d, 10.0f);
  assert((float)d.engineTemperature > before);
}

static void test_engine_temp_never_negative(void) {
  sensorData d;
  initSensorData(&d);
  d.engineTemp = -50.0f;

  sensorDataUpdate(&d, 100.0f);
  assert(d.engineTemperature >= 0);
}

static void test_engine_temp_zero_time_step_is_noop(void) {
  sensorData d;
  initSensorData(&d);
  d.engineTemp = 85.0f;

  sensorDataUpdate(&d, 5.0f);
  int temp = d.engineTemperature;
  sensorDataUpdate(&d, 5.0f); // same timestamp -> dt = 0 -> unchanged
  assert(d.engineTemperature == temp);
}

int main() {
  sensor_init_test();
  sensor_data_init_test();
  test_update_fields_in_range();
  test_consecutive_updates_differ();
  test_update_is_deterministic_with_seed();
  test_fuel_depletes_over_time();
  test_fuel_never_goes_negative();
  test_to_string_contains_sensor_id();
  test_engine_temp_cools_when_above_equilibrium();
  test_engine_temp_heats_when_below_equilibrium();
  test_engine_temp_never_negative();
  test_engine_temp_zero_time_step_is_noop();
  printf("All sensor tests passed\n");
  return 0;
}