#include "../Queue/queue.h"
#include "../sensor/sensor.h"
#include <assert.h>
#include <stdio.h>

// Helper: sensor carrying a single "level" metric, used as a
// distinguishing value across queue operations.
static sensorData make_sensor_data(float level) {
  sensorData d;
  initSensorData(&d);
  addMetric(&d, "level", "%", level);
  return d;
}

static float level_of(sensorData d) {
  Metric* m = findMetric(&d, "level");
  assert(m != NULL);
  return m->value;
}

static void test_init_sets_size_and_capacity(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  assert(sq.current_size == 0);
  assert(sq.allocated_size == 100);
}

static void test_add_increases_size(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  addElemToQueue(&sq, make_sensor_data(1));
  assert(sq.current_size == 1);
  addElemToQueue(&sq, make_sensor_data(2));
  assert(sq.current_size == 2);
}

static void test_remove_keeps_fifo_order(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  addElemToQueue(&sq, make_sensor_data(1));
  addElemToQueue(&sq, make_sensor_data(2));
  addElemToQueue(&sq, make_sensor_data(3));

  assert(level_of(removeElemFromQueue(&sq)) == 1);
  assert(sq.current_size == 2);
  assert(level_of(removeElemFromQueue(&sq)) == 2);
  assert(sq.current_size == 1);
  assert(level_of(removeElemFromQueue(&sq)) == 3);
  assert(sq.current_size == 0);
}

static void test_remove_shifts_remaining_elements(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  for (int i = 1; i <= 5; i++) addElemToQueue(&sq, make_sensor_data(i));

  assert(level_of(removeElemFromQueue(&sq)) == 1);
  assert(level_of(sq.data[0]) == 2);
  assert(level_of(sq.data[sq.current_size - 1]) == 5);
}

static void test_remove_from_empty_returns_zeroed_data(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  sensorData removed = removeElemFromQueue(&sq);
  assert(removed.metricCount == 0);
  assert(findMetric(&removed, "level") == NULL);
  assert(sq.current_size == 0);
}

static void test_growth_when_capacity_reached(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  for (int i = 1; i <= 250; i++) addElemToQueue(&sq, make_sensor_data(i));

  assert(sq.current_size == 250);
  assert(sq.allocated_size > 100);

  assert(level_of(removeElemFromQueue(&sq)) == 1);
  assert(level_of(sq.data[sq.current_size - 1]) == 250);
}

static void test_is_empty_queue(void) {
  sensorQueue sq;
  initSensorQueue(&sq);
  assert(isSensorQueueEmpty(&sq));
  addElemToQueue(&sq, make_sensor_data(10));
  assert(!isSensorQueueEmpty(&sq));
}

static void test_destroy_resets_queue(void) {
  sensorQueue sq;
  initSensorQueue(&sq);

  addElemToQueue(&sq, make_sensor_data(10));
  destroySensorQueue(&sq);

  assert(sq.data == NULL);
  assert(sq.current_size == 0);
  assert(sq.allocated_size == 0);

  destroySensorQueue(&sq);
}

int main(void) {
  test_init_sets_size_and_capacity();
  test_add_increases_size();
  test_remove_keeps_fifo_order();
  test_remove_shifts_remaining_elements();
  test_remove_from_empty_returns_zeroed_data();
  test_growth_when_capacity_reached();
  test_is_empty_queue();
  test_destroy_resets_queue();
  printf("All queue tests passed\n");
  return 0;
}
