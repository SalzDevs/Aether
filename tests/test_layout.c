#include "../ui/layout.h"
#include <assert.h>
#include <stdio.h>

// ---------- layoutRound2 ----------

static void test_round2_basic(void) {
  assert(layoutRound2(22.567f) == 22.57f);
  assert(layoutRound2(45.0f) == 45.0f);
  assert(layoutRound2(0.004f) == 0.0f);
  assert(layoutRound2(1013.256f) == 1013.26f);
}

static void test_round2_negative(void) {
  assert(layoutRound2(-1.004f) == -1.0f);
  assert(layoutRound2(-2.345f) == -2.35f); // rounds away from zero at .5
}

// ---------- layoutMapRange ----------

static void test_map_range_endpoints(void) {
  // higher value maps to the top (smaller y)
  assert(layoutMapRange(0.0f, 0.0f, 10.0f, 100.0f, 0.0f) == 100.0f);
  assert(layoutMapRange(10.0f, 0.0f, 10.0f, 100.0f, 0.0f) == 0.0f);
  assert(layoutMapRange(5.0f, 0.0f, 10.0f, 100.0f, 0.0f) == 50.0f);
}

static void test_map_range_flat_series_hits_midpoint(void) {
  assert(layoutMapRange(7.0f, 7.0f, 7.0f, 10.0f, 30.0f) == 20.0f);
}

static void test_map_range_identity(void) {
  assert(layoutMapRange(3.0f, 0.0f, 10.0f, 0.0f, 10.0f) == 3.0f);
}

// ---------- layoutMinMax ----------

static void test_min_max_basic(void) {
  float values[] = { 4.0f, -2.0f, 9.5f, 3.0f };
  float min, max;
  layoutMinMax(values, 4, &min, &max);
  assert(min == -2.0f);
  assert(max == 9.5f);
}

static void test_min_max_single_and_empty(void) {
  float single[] = { 5.0f };
  float min, max;
  layoutMinMax(single, 1, &min, &max);
  assert(min == 5.0f && max == 5.0f);

  layoutMinMax(NULL, 0, &min, &max);
  assert(min == 0.0f && max == 0.0f);
}

// ---------- layoutTabs ----------

static void test_tabs_small_window_six_per_tab(void) {
  // 800x450 window: 768x400 available -> 3 cols x 2 rows = 6 per tab
  TabLayout tl = layoutTabs(768, 400, 230.0f, 160.0f, 30);
  assert(tl.cols == 3);
  assert(tl.rows == 2);
  assert(tl.perTab == 6);
  assert(tl.pageCount == 5);
  assert(tl.pagerVisible);
  assert(tl.bottomReserved > 0);
}

static void test_tabs_single_page_hides_pager(void) {
  TabLayout tl = layoutTabs(768, 400, 230.0f, 160.0f, 5);
  assert(tl.pageCount == 1);
  assert(!tl.pagerVisible);
  assert(tl.bottomReserved == 0);
}

static void test_tabs_large_window_more_per_tab(void) {
  // 1280x800 window: 1248x750 available -> 5 cols x 4 rows = 20 per tab
  TabLayout tl = layoutTabs(1248, 750, 230.0f, 160.0f, 30);
  assert(tl.cols == 5);
  assert(tl.perTab == 20);
  assert(tl.pageCount == 2);
}

static void test_tabs_narrow_window_single_column(void) {
  TabLayout tl = layoutTabs(200, 400, 230.0f, 160.0f, 10);
  assert(tl.cols == 1);
  assert(tl.pageCount == (10 + tl.perTab - 1) / tl.perTab);
}

static void test_tabs_empty_registry(void) {
  TabLayout tl = layoutTabs(768, 400, 230.0f, 160.0f, 0);
  assert(tl.pageCount == 0);
  assert(!tl.pagerVisible);
}

// ---------- layoutPageList ----------

static bool seq_equals(const int* got, int n, const int* expected, int expectedN) {
  if (n != expectedN) return false;
  for (int i = 0; i < n; i++) {
    if (got[i] != expected[i]) return false;
  }
  return true;
}

static void test_page_list_all_pages_when_few(void) {
  int out[9];
  int n = layoutPageList(5, 1, out, 9);
  int expected[] = { 1, 2, 3, 4, 5 };
  assert(seq_equals(out, n, expected, 5));
}

static void test_page_list_collapses_first_page(void) {
  int out[9];
  int n = layoutPageList(10, 1, out, 9);
  int expected[] = { 1, 2, -1, 10 };
  assert(seq_equals(out, n, expected, 4));
}

static void test_page_list_collapses_middle_page(void) {
  int out[9];
  int n = layoutPageList(10, 5, out, 9);
  int expected[] = { 1, -1, 4, 5, 6, -1, 10 };
  assert(seq_equals(out, n, expected, 7));
}

static void test_page_list_collapses_last_page(void) {
  int out[9];
  int n = layoutPageList(10, 10, out, 9);
  int expected[] = { 1, -1, 9, 10 };
  assert(seq_equals(out, n, expected, 4));
}

static void test_page_list_never_exceeds_out_max(void) {
  for (int pages = 1; pages <= 60; pages++) {
    for (int cur = 1; cur <= pages; cur++) {
      int out[9];
      int n = layoutPageList(pages, cur, out, 9);
      assert(n > 0 && n <= 9);
      assert(out[0] == 1);            // first page always present
      assert(out[n - 1] == pages);    // last page always present
      // current page always visible
      bool found = false;
      for (int i = 0; i < n; i++) {
        if (out[i] == cur) found = true;
        assert(out[i] != 0);          // no uninitialized slots
      }
      assert(found);
    }
  }
}

int main(void) {
  test_round2_basic();
  test_round2_negative();
  test_map_range_endpoints();
  test_map_range_flat_series_hits_midpoint();
  test_map_range_identity();
  test_min_max_basic();
  test_min_max_single_and_empty();
  test_tabs_small_window_six_per_tab();
  test_tabs_single_page_hides_pager();
  test_tabs_large_window_more_per_tab();
  test_tabs_narrow_window_single_column();
  test_tabs_empty_registry();
  test_page_list_all_pages_when_few();
  test_page_list_collapses_first_page();
  test_page_list_collapses_middle_page();
  test_page_list_collapses_last_page();
  test_page_list_never_exceeds_out_max();
  printf("All layout tests passed\n");
  return 0;
}
