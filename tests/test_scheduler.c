#include "../scheduler/scheduler.h"
#include <assert.h>
#include <stdio.h>

static void dummy_function(void *ctx) { (void)ctx; }

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

int main(void) {
  test_init_sets_period_and_function();
  test_init_zero_values();
  test_should_run_exactly_at_period();
  test_should_not_run_before_period();
  test_should_run_long_past_period();
  test_zero_period_always_ready();
  test_wrap_around_still_runs();
  printf("All scheduler tests passed\n");
  return 0;
}
