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

int main() {
  sensor_init_test();
  sensor_data_init_test();
  test_update_fields_in_range();
  test_consecutive_updates_differ();
  test_update_is_deterministic_with_seed();
  test_fuel_depletes_over_time();
  test_fuel_never_goes_negative();
  test_to_string_contains_sensor_id();
  printf("All sensor tests passed\n");
  return 0;
}