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

static void test_run_task_executes_and_updates_last_run_when_period_elapsed(void) {
  Task t;
  uint64_t start = (uint64_t)time(NULL) - 3;
  initTask(&t, 3, dummy_function, NULL, start);
  dummy_calls = 0;
  runTask(&t);

  assert(dummy_calls == 1);
  assert(t.last_run >= start + 3); // last_run moved forward
}

static void test_run_task_does_nothing_before_period(void) {
  Task t;
  uint64_t start = (uint64_t)time(NULL);
  initTask(&t, 3, dummy_function, NULL, start);
  dummy_calls = 0;
  runTask(&t);

  assert(dummy_calls == 0);
  assert(t.last_run == start); // unchanged
}

static void test_run_task_with_null_task_does_not_crash(void) {
  runTask(NULL);
  runTask(&(Task){ .function = NULL });
}

int main(void) {
  test_init_sets_period_and_function();
  test_init_zero_values();
  test_should_run_exactly_at_period();
  test_should_not_run_before_period();
  test_should_run_long_past_period();
  test_zero_period_always_ready();
  test_wrap_around_still_runs();
  test_run_task_executes_and_updates_last_run_when_period_elapsed();
  test_run_task_does_nothing_before_period();
  test_run_task_with_null_task_does_not_crash();
  printf("All scheduler tests passed\n");
  return 0;
}
