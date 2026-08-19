#include "../config/config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_file_to_sensors(void) {
  size_t count = 0;
  sensorData* sensors = fileToSensors("config/sensors.yaml", &count);

  assert(sensors != NULL);
  assert(count == 5);

  assert(sensors[0].sensorId == 1);
  assert(sensors[0].airSpeed == 120.5f);
  assert(sensors[0].altitude == 1000.0f);
  assert(sensors[0].engineTemperature == 85);
  assert(sensors[0].fuelLevel == 60);
  assert(sensors[0].batteryVoltage == 12);

  assert(sensors[3].sensorId == 4);
  assert(sensors[3].fuelLevel == 42);

  assert(sensors[4].sensorId == 5);
  assert(sensors[4].airSpeed == 115.8f);
  assert(sensors[4].altitude == 1100.0f);
  assert(sensors[4].engineTemperature == 87);
  assert(sensors[4].fuelLevel == 42);
  assert(sensors[4].batteryVoltage == 12);

  free(sensors);
}

static void test_file_to_sensors_missing_file(void) {
  size_t count = 0;
  sensorData* sensors = fileToSensors("config/does_not_exist.yaml", &count);

  assert(sensors == NULL);
  assert(count == 0);
}

int main() {
  test_file_to_sensors();
  test_file_to_sensors_missing_file();
  printf("All config tests passed\n");
  return 0;
}
