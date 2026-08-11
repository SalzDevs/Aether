#include "../Qeue/qeue.h"
#include "../sensor/sensor.h"
#include <assert.h>
#include <stdio.h>

static sensor make_sensor(int fuel_level) {
  sensor s;
  initSensor(&s);
  s.fuelLevel = fuel_level;
  return s;
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
  addElemToQueue(&sq, make_sensor(1));
  assert(sq.current_size == 1);
  addElemToQueue(&sq, make_sensor(2));
  assert(sq.current_size == 2);
}

static void test_remove_keeps_fifo_order(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  addElemToQueue(&sq, make_sensor(1));
  addElemToQueue(&sq, make_sensor(2));
  addElemToQueue(&sq, make_sensor(3));

  assert(removeElemFromQeue(&sq).fuelLevel == 1);
  assert(sq.current_size == 2);
  assert(removeElemFromQeue(&sq).fuelLevel == 2);
  assert(sq.current_size == 1);
  assert(removeElemFromQeue(&sq).fuelLevel == 3);
  assert(sq.current_size == 0);
}

static void test_remove_shifts_remaining_elements(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  for (int i = 1; i <= 5; i++) addElemToQueue(&sq, make_sensor(i));

  assert(removeElemFromQeue(&sq).fuelLevel == 1);
  assert(sq.data[0].fuelLevel == 2);
  assert(sq.data[sq.current_size - 1].fuelLevel == 5);
}

static void test_remove_from_empty_returns_zeroed_sensor(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  sensor removed = removeElemFromQeue(&sq);
  assert(removed.airSpeed == 0.0);
  assert(removed.fuelLevel == 0);
  assert(sq.current_size == 0);
}

static void test_growth_when_capacity_reached(void) {
  sensorQeue sq;
  initSensorQeue(&sq);
  for (int i = 1; i <= 250; i++) addElemToQueue(&sq, make_sensor(i));

  assert(sq.current_size == 250);
  assert(sq.allocated_size > 100);

  assert(removeElemFromQeue(&sq).fuelLevel == 1);
  assert(sq.data[sq.current_size - 1].fuelLevel == 250);
}

int main(void) {
  test_init_sets_size_and_capacity();
  test_add_increases_size();
  test_remove_keeps_fifo_order();
  test_remove_shifts_remaining_elements();
  test_remove_from_empty_returns_zeroed_sensor();
  test_growth_when_capacity_reached();
  printf("All qeue tests passed\n");
  return 0;
}