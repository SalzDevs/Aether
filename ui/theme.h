#ifndef THEME_H
#define THEME_H

#include "raylib.h"

// Central design tokens for the dashboard.
// All visual identity decisions live here.

typedef struct {
  // Palette
  Color background; // window background
  Color panel;      // sensor card fill
  Color titleText;  // headers, app title, sensor names
  Color labelText;  // metric names, units, secondary info
  Color valueText;  // metric readings
  Color divider;    // hairlines inside cards

  // Spacing
  int padding;      // outer margin + inner card padding
  int cardGap;      // space between cards
  int topBarHeight; // reserved strip above the grid

  // Typography (base atlas size the font is loaded at)
  float fontAtlasSize;
} DashboardTheme;

extern const DashboardTheme THEME;

#endif
