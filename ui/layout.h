#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdbool.h>
#include <stddef.h>

// Pure layout and formatting math used by the dashboard.
// No rendering dependencies, so it is unit-testable in isolation.

// Rounds to two decimals so animated values render compactly.
float layoutRound2(float v);

// Maps value from [inMin, inMax] to [outMin, outMax].
// When inMin == inMax, returns the midpoint of the output range.
float layoutMapRange(float value, float inMin, float inMax,
                     float outMin, float outMax);

// Computes min and max of a value series.
void layoutMinMax(const float* values, size_t count,
                  float* outMin, float* outMax);

// Result of laying out the sensor tab grid.
typedef struct {
  int cols;
  int rows;
  int perTab;         // cards per tab (cols * rows)
  int pageCount;      // total tabs needed
  bool pagerVisible;  // true when more than one tab is needed
  int bottomReserved; // pixels reserved at the bottom for the pager
} TabLayout;

// Fits as many minimum-size cards as possible into the available area.
// When sensors exceed one tab, space is reserved for the bottom pager.
TabLayout layoutTabs(int availWidth, int availHeight,
                     float minCardWidth, float minCardHeight,
                     size_t sensorCount);

// Builds the visible tab sequence with collapsing: first and last pages
// are always shown, the current page +-1, and -1 markers in larger gaps.
// pageCount <= outMax lists every page. Returns the number of slots used.
int layoutPageList(int pageCount, int currentPage1Based,
                   int* out, int outMax);

#endif
