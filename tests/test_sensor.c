#include "../sensor/sensor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static void sensor_init_test() {
  sensor s;
  initSensor(&s);
  assert(s.airSpeed == 0.0);
  assert(s.altitude == 0.0);
  assert(s.engineTemperature == 0);
  assert(s.fuelLevel == 0);
  assert(s.baterryVoltage == 0);
}

static void test_update_fields_in_range(void) {
  sensor s;
  initSensor(&s);
  sensorDataUpdate(&s);
  assert(s.airSpeed >= 0 && s.airSpeed <= RAND_MAX);
  assert(s.altitude >= 0 && s.altitude <= RAND_MAX);
  assert(s.engineTemperature >= 0 && s.engineTemperature <= RAND_MAX);
  assert(s.fuelLevel >= 0 && s.fuelLevel <= RAND_MAX);
  assert(s.baterryVoltage >= 0 && s.baterryVoltage <= RAND_MAX);
}

static void test_consecutive_updates_differ(void) {
  sensor s;
  initSensor(&s);
  sensorDataUpdate(&s);
  float speed1 = s.airSpeed;
  sensorDataUpdate(&s);
  assert(s.airSpeed != speed1);
}

static void test_update_is_deterministic_with_seed(void) {
  sensor s1, s2;
  initSensor(&s1);
  initSensor(&s2);

  srand(42);
  sensorDataUpdate(&s1);

  srand(42);
  sensorDataUpdate(&s2);

  assert(s1.airSpeed == s2.airSpeed);
  assert(s1.altitude == s2.altitude);
  assert(s1.engineTemperature == s2.engineTemperature);
  assert(s1.fuelLevel == s2.fuelLevel);
  assert(s1.baterryVoltage == s2.baterryVoltage);
}

int main() {
  sensor_init_test();
  test_update_fields_in_range();
  test_consecutive_updates_differ();
  test_update_is_deterministic_with_seed();
  return 0;
}
