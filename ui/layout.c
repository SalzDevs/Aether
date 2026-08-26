#include "layout.h"

float layoutRound2(float v) {
  long scaled = (long)(v * 100.0f + (v >= 0.0f ? 0.5f : -0.5f));
  return (float)scaled / 100.0f;
}

float layoutMapRange(float value, float inMin, float inMax,
                     float outMin, float outMax) {
  if (inMax > inMin) {
    float t = (value - inMin) / (inMax - inMin);
    return outMin + t * (outMax - outMin);
  }
  return (outMin + outMax) / 2.0f;
}

void layoutMinMax(const float* values, size_t count,
                  float* outMin, float* outMax) {
  if (count == 0 || values == NULL) {
    *outMin = 0.0f;
    *outMax = 0.0f;
    return;
  }
  float min = values[0], max = values[0];
  for (size_t i = 1; i < count; i++) {
    if (values[i] < min) min = values[i];
    if (values[i] > max) max = values[i];
  }
  *outMin = min;
  *outMax = max;
}

TabLayout layoutTabs(int availWidth, int availHeight,
                     float minCardWidth, float minCardHeight,
                     size_t sensorCount) {
  TabLayout tl = { 0 };

  int cols = (int)(availWidth / minCardWidth);
  if (cols < 1) cols = 1;
  int rowsFull = (int)(availHeight / minCardHeight);
  if (rowsFull < 1) rowsFull = 1;

  int perTabFull = cols * rowsFull;
  bool pagerNeeded = (int)sensorCount > perTabFull;

  int rows = rowsFull;
  int bottomReserved = 0;
  if (pagerNeeded) {
    bottomReserved = 34; // bottom strip for the pager
    rows = (int)((availHeight - bottomReserved) / minCardHeight);
    if (rows < 1) rows = 1;
  }

  int perTab = cols * rows;
  int pageCount = ((int)sensorCount + perTab - 1) / perTab;

  tl.cols = cols;
  tl.rows = rows;
  tl.perTab = perTab;
  tl.pageCount = pageCount;
  tl.pagerVisible = pagerNeeded;
  tl.bottomReserved = bottomReserved;
  return tl;
}

int layoutPageList(int pageCount, int currentPage1Based,
                   int* out, int outMax) {
  int n = 0;
  if (pageCount <= outMax) {
    for (int p = 1; p <= pageCount; p++) out[n++] = p;
    return n;
  }

  int candidates[5] = { 1, currentPage1Based - 1, currentPage1Based,
                        currentPage1Based + 1, pageCount };
  int prev = 0;
  for (int i = 0; i < 5; i++) {
    int p = candidates[i];
    if (p < 1 || p > pageCount) continue;
    if (n > 0 && p == prev) continue; // dedupe (candidates are sorted)
    if (n > 0 && p - prev > 1 && n < outMax - 1) out[n++] = -1; // gap marker
    if (n >= outMax) break;
    out[n++] = p;
    prev = p;
  }

  // guarantee the last page made it (drop a gap marker if needed)
  while (n > 0 && out[n - 1] != pageCount) {
    if (out[n - 1] == -1) { n--; continue; }
    if (n < outMax) { out[n++] = pageCount; break; }
    n -= 2; // drop marker + neighbor to make room
  }
  return n;
}
