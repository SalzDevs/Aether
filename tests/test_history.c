#include "../History/history.h"
#include "../sensor/sensor.h"
#include <assert.h>
#include <stdio.h>

// Helper: sensor carrying a single "level" metric, used as a
// distinguishing value across history operations.
static sensorData make_sensor_data(float level) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "level", "%", level);
  return d;
}

static float level_of(const sensorData* d) {
  const Metric* m = findMetric((sensorData*)d, "level");
  assert(m != NULL);
  return m->value;
}

static void test_init_sets_capacity_and_empty(void) {
  sensorHistory h;
  initSensorHistory(&h, 3);
  assert(h.capacity == 3);
  assert(sensorHistorySize(&h) == 0);
  assert(sensorHistoryAt(&h, 0) == NULL);
  destroySensorHistory(&h);
}

static void test_push_increases_size(void) {
  sensorHistory h;
  initSensorHistory(&h, 3);
  pushSensorReading(&h, make_sensor_data(1));
  assert(sensorHistorySize(&h) == 1);
  pushSensorReading(&h, make_sensor_data(2));
  assert(sensorHistorySize(&h) == 2);
  destroySensorHistory(&h);
}

static void test_readings_ordered_oldest_to_newest(void) {
  sensorHistory h;
  initSensorHistory(&h, 8);
  for (int i = 1; i <= 5; i++) pushSensorReading(&h, make_sensor_data((float)i));

  assert(sensorHistorySize(&h) == 5);
  for (int i = 0; i < 5; i++) {
    assert(level_of(sensorHistoryAt(&h, (size_t)i)) == (float)(i + 1));
  }
  destroySensorHistory(&h);
}

static void test_full_history_drops_oldest(void) {
  sensorHistory h;
  initSensorHistory(&h, 3);
  for (int i = 1; i <= 6; i++) pushSensorReading(&h, make_sensor_data((float)i));

  // capacity 3: readings 1..3 were dropped, 4..6 remain
  assert(sensorHistorySize(&h) == 3);
  assert(level_of(sensorHistoryAt(&h, 0)) == 4);
  assert(level_of(sensorHistoryAt(&h, 1)) == 5);
  assert(level_of(sensorHistoryAt(&h, 2)) == 6);
  destroySensorHistory(&h);
}

static void test_ring_wraps_many_times(void) {
  sensorHistory h;
  initSensorHistory(&h, 4);

  // push far beyond capacity, in a prime-ish count to hit all slots
  for (int i = 1; i <= 11; i++) pushSensorReading(&h, make_sensor_data((float)i));

  assert(sensorHistorySize(&h) == 4);
  assert(level_of(sensorHistoryAt(&h, 0)) == 8);
  assert(level_of(sensorHistoryAt(&h, 1)) == 9);
  assert(level_of(sensorHistoryAt(&h, 2)) == 10);
  assert(level_of(sensorHistoryAt(&h, 3)) == 11);
  destroySensorHistory(&h);
}

static void test_at_out_of_range_returns_null(void) {
  sensorHistory h;
  initSensorHistory(&h, 2);
  pushSensorReading(&h, make_sensor_data(1));
  assert(sensorHistoryAt(&h, 1) == NULL);
  assert(sensorHistoryAt(&h, 99) == NULL);
  destroySensorHistory(&h);
}

static void test_destroy_resets_state(void) {
  sensorHistory h;
  initSensorHistory(&h, 2);
  pushSensorReading(&h, make_sensor_data(1));
  destroySensorHistory(&h);

  assert(h.data == NULL);
  assert(h.size == 0);
  assert(h.capacity == 0);

  destroySensorHistory(&h); // double destroy is safe
}

static void test_zero_capacity_is_clamped_to_one(void) {
  sensorHistory h;
  initSensorHistory(&h, 0);
  assert(h.capacity == 1);
  pushSensorReading(&h, make_sensor_data(1));
  pushSensorReading(&h, make_sensor_data(2));
  assert(sensorHistorySize(&h) == 1);
  assert(level_of(sensorHistoryAt(&h, 0)) == 2);
  destroySensorHistory(&h);
}

int main(void) {
  test_init_sets_capacity_and_empty();
  test_push_increases_size();
  test_readings_ordered_oldest_to_newest();
  test_full_history_drops_oldest();
  test_ring_wraps_many_times();
  test_at_out_of_range_returns_null();
  test_destroy_resets_state();
  test_zero_capacity_is_clamped_to_one();
  printf("All history tests passed\n");
  return 0;
}
