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
  Color upColor;    // indicator: reading increased
  Color downColor;  // indicator: reading decreased
  Color divider;    // hairlines inside cards

  // Spacing
  int padding;      // outer margin + inner card padding
  int cardGap;      // space between cards
  int topBarHeight; // reserved strip above the grid

  // Typography (base atlas size the font is loaded at)
  float fontAtlasSize;

  // Icons
  Color iconColor;      // idle icon tint (dim)
  Color iconHoverColor; // icon tint while hovered

  // Motion
  float arrowHoldSeconds; // how long up/down indicators stay visible
  float valueSmoothing;   // how fast displayed values chase actual ones (per second)
} DashboardTheme;

extern const DashboardTheme THEME;

#endif
