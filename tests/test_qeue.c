#include "../Qeue/qeue.h"
#include "../sensor/sensor.h"
#include <assert.h>
#include <stdio.h>

// Helper: sensor carrying a single "fuel" metric, used as a
// distinguishing value across queue operations.
static sensorData make_sensor_data(int fuel_level) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "fuel", "%", (float)fuel_level);
  return d;
}

static float fuel_of(sensorData d) {
  Metric* m = findMetric(&d, "fuel");
  assert(m != NULL);
  return m->value;
}

static void test_init_sets_size_and_capacity(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  assert(sq.current_size == 0);
  assert(sq.allocated_size == 100);
}

static void test_add_increases_size(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  addElemToQueue(&sq, make_sensor_data(1));
  assert(sq.current_size == 1);
  addElemToQueue(&sq, make_sensor_data(2));
  assert(sq.current_size == 2);
}

static void test_remove_keeps_fifo_order(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  addElemToQueue(&sq, make_sensor_data(1));
  addElemToQueue(&sq, make_sensor_data(2));
  addElemToQueue(&sq, make_sensor_data(3));

  assert(fuel_of(removeElemFromQeue(&sq)) == 1);
  assert(sq.current_size == 2);
  assert(fuel_of(removeElemFromQeue(&sq)) == 2);
  assert(sq.current_size == 1);
  assert(fuel_of(removeElemFromQeue(&sq)) == 3);
  assert(sq.current_size == 0);
}

static void test_remove_shifts_remaining_elements(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  for (int i = 1; i <= 5; i++) addElemToQueue(&sq, make_sensor_data(i));

  assert(fuel_of(removeElemFromQeue(&sq)) == 1);
  assert(fuel_of(sq.data[0]) == 2);
  assert(fuel_of(sq.data[sq.current_size - 1]) == 5);
}

static void test_remove_from_empty_returns_zeroed_data(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  sensorData removed = removeElemFromQeue(&sq);
  assert(removed.metricCount == 0);
  assert(findMetric(&removed, "fuel") == NULL);
  assert(sq.current_size == 0);
}

static void test_growth_when_capacity_reached(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  for (int i = 1; i <= 250; i++) addElemToQueue(&sq, make_sensor_data(i));

  assert(sq.current_size == 250);
  assert(sq.allocated_size > 100);

  assert(fuel_of(removeElemFromQeue(&sq)) == 1);
  assert(fuel_of(sq.data[sq.current_size - 1]) == 250);
}

static void test_is_empty_qeue(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  assert(isSensorQeueEmpty(&sq));
  addElemToQueue(&sq, make_sensor_data(10));
  assert(!isSensorQeueEmpty(&sq));
}

static void test_destroy_resets_queue(void) {
  sensorQeue sq;
  initSensorQeue(&sq);

  addElemToQueue(&sq, make_sensor_data(10));
  destroySensorQeue(&sq);

  assert(sq.data == NULL);
  assert(sq.current_size == 0);
  assert(sq.allocated_size == 0);

  destroySensorQeue(&sq);
}

int main(void) {
  test_init_sets_size_and_capacity();
  test_add_increases_size();
  test_remove_keeps_fifo_order();
  test_remove_shifts_remaining_elements();
  test_remove_from_empty_returns_zeroed_data();
  test_growth_when_capacity_reached();
  test_is_empty_qeue();
  test_destroy_resets_queue();
  printf("All qeue tests passed\n");
  return 0;
}
