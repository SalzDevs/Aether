#include "../scheduler/scheduler.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

static int dummy_calls = 0;

static void dummy_function(void *ctx) { (void)ctx; dummy_calls++; }

static void test_init_sets_period_and_function(void) {
  Task t;
  initTask(&t, 50, dummy_function, NULL, 12345);
  assert(t.period == 50);
  assert(t.function == dummy_function);
  assert(t.last_run == 12345);
}

static void test_init_zero_values(void) {
  Task t;
  initTask(&t, 0, NULL, NULL, 0);
  assert(t.period == 0);
  assert(t.function == NULL);
  assert(t.last_run == 0);
}

static void test_should_run_exactly_at_period(void) {
  assert(shouldRunTask(30, 1000, 1030));
}

static void test_should_not_run_before_period(void) {
  assert(!shouldRunTask(30, 1000, 1029));
}

static void test_should_run_long_past_period(void) {
  assert(shouldRunTask(30, 1000, 5000));
}

static void test_zero_period_always_ready(void) {
  assert(shouldRunTask(0, 1000, 1000));
}

static void test_wrap_around_still_runs(void) {
  assert(shouldRunTask(30, 1000, 10));
}

static sensorQueue make_queue_with_sensors(size_t count) {
  sensorQueue sq;
  initSensorQueue(&sq);
  for (size_t i = 1; i <= count; i++) {
    sensorData d;
    initSensorData(&d);
    d.id = (int)i;
    addElemToQueue(&sq, d);
  }
  return sq;
}

static void test_run_task_executes_and_removes_front_when_period_elapsed(void) {
  sensorQueue sq = make_queue_with_sensors(2);

  Task t;
  initTask(&t, 3, dummy_function, NULL, (uint64_t)time(NULL) - 3);
  dummy_calls = 0;
  runTask(&t, &sq);

  assert(dummy_calls == 1);
  assert(sq.current_size == 1);
  assert(sq.data[0].id == 2);
}

static void test_run_task_does_nothing_before_period(void) {
  sensorQueue sq = make_queue_with_sensors(2);

  Task t;
  initTask(&t, 3, dummy_function, NULL, (uint64_t)time(NULL));
  dummy_calls = 0;
  runTask(&t, &sq);

  assert(dummy_calls == 0);
  assert(sq.current_size == 2);
}

static void test_run_task_on_empty_queue_does_not_crash(void) {
  sensorQueue sq;
  initSensorQueue(&sq);

  Task t;
  initTask(&t, 0, dummy_function, NULL, (uint64_t)time(NULL));
  dummy_calls = 0;
  runTask(&t, &sq);

  assert(dummy_calls == 1);
  assert(sq.current_size == 0);
}

int main(void) {
  test_init_sets_period_and_function();
  test_init_zero_values();
  test_should_run_exactly_at_period();
  test_should_not_run_before_period();
  test_should_run_long_past_period();
  test_zero_period_always_ready();
  test_wrap_around_still_runs();
  test_run_task_executes_and_removes_front_when_period_elapsed();
  test_run_task_does_nothing_before_period();
  test_run_task_on_empty_queue_does_not_crash();
  printf("All scheduler tests passed\n");
  return 0;
}
