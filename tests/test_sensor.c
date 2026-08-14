#include "../sensor/sensor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

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
  assert(d.baterryVoltage == 0);
}

static void test_update_fields_in_range(void) {
  sensorData d;
  initSensorData(&d);
  sensorDataUpdate(&d);
  assert(d.airSpeed >= 0 && d.airSpeed <= (float)RAND_MAX);
  assert(d.altitude >= 0 && d.altitude <= (float)RAND_MAX);
  assert(d.engineTemperature >= 0 && d.engineTemperature <= RAND_MAX);
  assert(d.fuelLevel >= 0 && d.fuelLevel <= RAND_MAX);
  assert(d.baterryVoltage >= 0 && d.baterryVoltage <= RAND_MAX);
}

static void test_consecutive_updates_differ(void) {
  sensorData d;
  initSensorData(&d);
  sensorDataUpdate(&d);
  float speed1 = d.airSpeed;
  sensorDataUpdate(&d);
  assert(d.airSpeed != speed1);
}

static void test_update_is_deterministic_with_seed(void) {
  sensorData d1, d2;
  initSensorData(&d1);
  initSensorData(&d2);

  srand(42);
  sensorDataUpdate(&d1);

  srand(42);
  sensorDataUpdate(&d2);

  assert(d1.airSpeed == d2.airSpeed);
  assert(d1.altitude == d2.altitude);
  assert(d1.engineTemperature == d2.engineTemperature);
  assert(d1.fuelLevel == d2.fuelLevel);
  assert(d1.baterryVoltage == d2.baterryVoltage);
}

int main() {
  sensor_init_test();
  sensor_data_init_test();
  test_update_fields_in_range();
  test_consecutive_updates_differ();
  test_update_is_deterministic_with_seed();
  printf("All sensor tests passed\n");
  return 0;
}