#include "dashboard.h"
#include "theme.h"
#include "../config/config.h"
#include "layout.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define FONT_PATH "assets/fonts/JetBrainsMono-Regular.ttf"

DashboardTheme THEME = {
    .background     = { 24, 26, 32, 255 },
    .panel          = { 33, 37, 47, 255 },
    .chip           = { 26, 29, 38, 255 },
    .titleText      = { 235, 240, 250, 255 },
    .labelText      = { 145, 152, 168, 255 },
    .valueText      = { 110, 190, 255, 255 },
    .upColor        = { 130, 205, 140, 255 },
    .downColor      = { 235, 135, 120, 255 },
    .divider        = { 52, 57, 70, 255 },
    .padding        = 16,
    .cardGap        = 12,
    .topBarHeight   = 34,
    .bottomBarHeight = 34,
    .minCardWidth   = 230.0f,
    .minCardHeight  = 160.0f,
    .fontAtlasSize  = 64.0f,
    .iconColor      = { 145, 152, 168, 255 },
    .iconHoverColor = { 235, 240, 250, 255 },
    .panelHoverColor = { 44, 49, 62, 255 },
    .arrowHoldSeconds = 1.5f,
    .valueSmoothing   = 10.0f,
};

const DashboardTheme THEME_PRESETS[] = {
    // 0: Dark (same as the original THEME)
    { { 24, 26, 32, 255 }, { 33, 37, 47, 255 }, { 26, 29, 38, 255 },
      { 235, 240, 250, 255 }, { 145, 152, 168, 255 }, { 110, 190, 255, 255 },
      { 130, 205, 140, 255 }, { 235, 135, 120, 255 }, { 52, 57, 70, 255 },
      16, 12, 34, 34, 230.0f, 160.0f, 64.0f,
      { 145, 152, 168, 255 }, { 235, 240, 250, 255 }, { 44, 49, 62, 255 },
      1.5f, 10.0f },

    // 1: Blue
    { { 22, 27, 34, 255 }, { 30, 38, 48, 255 }, { 24, 30, 38, 255 },
      { 232, 240, 250, 255 }, { 140, 158, 178, 255 }, { 90, 170, 255, 255 },
      { 120, 200, 150, 255 }, { 240, 140, 120, 255 }, { 48, 58, 72, 255 },
      16, 12, 34, 34, 230.0f, 160.0f, 64.0f,
      { 140, 158, 178, 255 }, { 232, 240, 250, 255 }, { 40, 50, 64, 255 },
      1.5f, 10.0f },

    // 2: Green
    { { 22, 30, 26, 255 }, { 30, 42, 36, 255 }, { 24, 34, 28, 255 },
      { 232, 248, 238, 255 }, { 140, 170, 155, 255 }, { 90, 215, 150, 255 },
      { 120, 225, 160, 255 }, { 240, 160, 120, 255 }, { 46, 64, 54, 255 },
      16, 12, 34, 34, 230.0f, 160.0f, 64.0f,
      { 140, 170, 155, 255 }, { 232, 248, 238, 255 }, { 40, 56, 46, 255 },
      1.5f, 10.0f },

    // 3: Amber
    { { 32, 28, 22, 255 }, { 44, 38, 30, 255 }, { 36, 31, 24, 255 },
      { 250, 244, 232, 255 }, { 178, 162, 140, 255 }, { 245, 190, 90, 255 },
      { 150, 210, 130, 255 }, { 240, 130, 110, 255 }, { 64, 56, 44, 255 },
      16, 12, 34, 34, 230.0f, 160.0f, 64.0f,
      { 178, 162, 140, 255 }, { 250, 244, 232, 255 }, { 56, 48, 38, 255 },
      1.5f, 10.0f },

    // 4: Purple
    { { 28, 24, 34, 255 }, { 38, 32, 46, 255 }, { 30, 26, 36, 255 },
      { 244, 238, 250, 255 }, { 162, 150, 178, 255 }, { 190, 140, 255, 255 },
      { 140, 215, 160, 255 }, { 240, 140, 170, 255 }, { 58, 50, 70, 255 },
      16, 12, 34, 34, 230.0f, 160.0f, 64.0f,
      { 162, 150, 178, 255 }, { 244, 238, 250, 255 }, { 50, 44, 62, 255 },
      1.5f, 10.0f },
};

const int THEME_PRESET_COUNT = sizeof(THEME_PRESETS)/sizeof(THEME);

static Settings g_settings = {
  .theme = 0,
  .showSparklines = true,
  .animateValues = true,
  .showIndicators = true,
};

void ApplyThemePreset(int index) {
  if (index < 0 || index >=THEME_PRESET_COUNT) {
    return;
  } 
  g_settings.theme = index;
  THEME = THEME_PRESETS[index];
}

void InitSettings(const char* path) {
  Settings s;
  LoadSettings(path, &s);          // falls back to defaults if file missing/invalid
  g_settings = s;
  ApplyThemePreset(g_settings.theme);
}


static Font g_font;
static bool g_fontLoaded = false;

// --- Animation state (one slot per metric, indexed sensor*MAX_METRICS+metric) ---
static float* g_display = NULL;   // smoothed value currently shown
static float* g_prev = NULL;      // last actual value seen (change detection)
static signed char* g_dir = NULL; // direction of last change: -1, 0, +1
static float* g_dirTimer = NULL;  // seconds left before the arrow fades
static size_t g_sensorCount = 0;

// --- Settings panel (pure UI state) ---
static bool g_settingsOpen = false;


// --- Detail view (pure UI state; index into the registry, -1 = closed) ---
static int g_detailSensor = -1;

// --- Tab pagination (pure UI state) ---
static int g_currentPage = 0;

void InitDashboard(const sensorData* sensors, size_t sensorCount) {
  // ESC must close the settings panel, not quit the app
  SetExitKey(KEY_NULL);

  size_t slots = sensorCount * MAX_METRICS;
  g_display = calloc(slots, sizeof(float));
  g_prev = calloc(slots, sizeof(float));
  g_dir = calloc(slots, sizeof(signed char));
  g_dirTimer = calloc(slots, sizeof(float));
  g_sensorCount = sensorCount;

  // Start display values at the actual readings (no animation on launch)
  for (size_t s = 0; s < sensorCount; s++) {
    for (size_t m = 0; m < sensors[s].metricCount; m++) {
      size_t idx = s * MAX_METRICS + m;
      g_display[idx] = sensors[s].metrics[m].value;
      g_prev[idx] = sensors[s].metrics[m].value;
    }
  }

  if (FileExists(FONT_PATH)) {
    // ASCII range plus the symbols units and labels use
    int codepoints[98];
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;
    codepoints[95] = 0xB7; // middle dot (stats separator)
    codepoints[96] = 0xB0; // degree sign
    codepoints[97] = 0xB5; // micro sign
    g_font = LoadFontEx(FONT_PATH, (int)THEME.fontAtlasSize, codepoints, 98);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
    g_fontLoaded = true;
  } else {
    // Fall back to raylib's built-in font; everything still works
    g_font = GetFontDefault();
    g_fontLoaded = false;
  }
}

void UnloadDashboard(void) {
  if (g_fontLoaded) UnloadFont(g_font);
  g_fontLoaded = false;
  free(g_display); free(g_prev); free(g_dir); free(g_dirTimer);
  g_display = NULL; g_prev = NULL; g_dir = NULL; g_dirTimer = NULL;
  g_sensorCount = 0;
}

// Advances value smoothing and change-indicator timers by dt seconds.
static void updateAnimations(const sensorData* sensors, float dt) {
  float alpha = dt * THEME.valueSmoothing;
  if (alpha > 1.0f) alpha = 1.0f;

  for (size_t s = 0; s < g_sensorCount; s++) {
    for (size_t m = 0; m < sensors[s].metricCount; m++) {
      size_t idx = s * MAX_METRICS + m;
      float target = sensors[s].metrics[m].value;

      // With animations off, displayed values snap to the readings
      g_display[idx] = g_settings.animateValues
          ? g_display[idx] + (target - g_display[idx]) * alpha
          : target;

      if (target != g_prev[idx]) {
        g_dir[idx] = target > g_prev[idx] ? 1 : -1;
        g_dirTimer[idx] = g_settings.showIndicators ? THEME.arrowHoldSeconds : 0.0f;
        g_prev[idx] = target;
      }
      if (g_dirTimer[idx] > 0.0f) g_dirTimer[idx] -= dt;
    }
  }
}

// Rounds to two decimals so animated values render compactly.
static float roundTo2(float v) {
  return layoutRound2(v);
}

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

// Text helpers: all rendering goes through the theme font with
// raylib's usual size/10 glyph spacing.
static Vector2 measureText(const char* text, float size) {
  return MeasureTextEx(g_font, text, size, size / 10.0f);
}

static void drawText(const char* text, int x, int y, float size, Color color) {
  DrawTextEx(g_font, text, (Vector2){ (float)x, (float)y }, size,
             size / 10.0f, color);
}

// Shrinks fontSize until text fits maxWidth (or hits the floor).
static void fitFontSize(const char* text, float maxWidth, float* fontSize, float floorSize) {
  Vector2 bounds = measureText(text, *fontSize);
  while (bounds.x > maxWidth && *fontSize > floorSize) {
    (*fontSize) -= 1.0f;
    bounds = measureText(text, *fontSize);
  }
}

// Draws a single line of text centered inside the given rect.
static void drawCenteredInRow(const char* text, int x, int y, int w, int h,
                              float fontSize, Color color) {
  Vector2 bounds = measureText(text, fontSize);
  drawText(text, x + ((int)w - (int)bounds.x) / 2, y + (h - (int)fontSize) / 2,
           fontSize, color);
}

// Draws the metric's reading history as a small trend line between
// the label and the value. Returns the width consumed (0 if skipped).
#define SPARK_MAX_POINTS 64

static int drawSparkline(const sensorHistory* hist, size_t metricIdx,
                         int x, int y, int maxW, int h) {
  if (hist == NULL || maxW < 24) return 0;

  size_t count = sensorHistorySize(hist);
  if (count < 2) return 0; // a trend needs at least two readings
  if (count > SPARK_MAX_POINTS) count = SPARK_MAX_POINTS;

  // Gather the series and its range
  float values[SPARK_MAX_POINTS];
  for (size_t i = 0; i < count; i++) {
    const sensorData* reading = sensorHistoryAt(hist, i);
    if (metricIdx >= reading->metricCount) return 0;
    values[i] = reading->metrics[metricIdx].value;
  }
  float min = 0.0f, max = 0.0f;
  layoutMinMax(values, count, &min, &max);

  // Vertical layout: small margins inside the row
  float top = (float)y + 4.0f;
  float bottom = (float)(y + h) - 4.0f;
  if (bottom - top < 4.0f) return 0;

  Vector2 points[SPARK_MAX_POINTS];
  float step = (float)maxW / (float)(count - 1);
  for (size_t i = 0; i < count; i++) {
    points[i] = (Vector2){ (float)x + step * (float)i,
                           layoutMapRange(values[i], min, max, bottom, top) };
  }

  Color lineColor = { THEME.valueText.r, THEME.valueText.g,
                      THEME.valueText.b, 110 };
  DrawLineStrip(points, (int)count, lineColor);

  // Bright dot on the newest reading
  Vector2 last = points[count - 1];
  DrawCircleV(last, 2.0f, THEME.valueText);

  return maxW;
}

// Draws one label/value line: label on the left, value + unit (+ change
// arrow) on the right. Sizes are computed by the caller.
static void drawLabelValueLine(const char* label, const char* valueBuf,
                               const char* unit, int x, int y, int w, int lineH,
                               int pad, float valueSize, float labelSize,
                               float unitSize, signed char dir,
                               float dirAlpha) {
  float groupWidth = measureText(valueBuf, valueSize).x + 4 +
                     measureText(unit, unitSize).x;
  int groupX = x + w - pad - (int)groupWidth;

  int labelY = y + (lineH - (int)labelSize) / 2;
  int valueY = y + (lineH - (int)valueSize) / 2;
  drawText(label, x + pad, labelY, labelSize, THEME.labelText);

  float arrowSize = valueSize / 3.0f;
  if (dir != 0 && dirAlpha > 0.0f && g_settings.showIndicators) {
    Color base = dir > 0 ? THEME.upColor : THEME.downColor;
    Color arrowColor = (Color){ base.r, base.g, base.b,
                                (unsigned char)(255 * dirAlpha) };
    float ax = (float)(groupX - (int)arrowSize - 6);
    float ay = (float)(valueY + (int)valueSize / 2);
    float half = arrowSize / 2.0f;
    if (dir > 0) {
      DrawTriangle((Vector2){ ax, ay + half },
                   (Vector2){ ax + arrowSize, ay + half },
                   (Vector2){ ax + half, ay - half }, arrowColor);
    } else {
      DrawTriangle((Vector2){ ax + arrowSize, ay - half },
                   (Vector2){ ax, ay - half },
                   (Vector2){ ax + half, ay + half }, arrowColor);
    }
  }

  drawText(valueBuf, groupX, valueY, valueSize, THEME.valueText);
  int unitX = groupX + (int)measureText(valueBuf, valueSize).x + 4;
  int unitY = valueY + (int)(valueSize - unitSize) / 3;
  drawText(unit, unitX, unitY, unitSize, THEME.labelText);
}

static void drawMetricRow(const Metric* m, int x, int y, int w, int h, int index,
                          float displayValue, signed char dir, float dirAlpha,
                          const sensorHistory* hist, size_t metricIdx,
                          float scale) {
  // Each metric is a two-line block: text line on top, sparkline
  // underneath. The sparkline is earned from leftover block height:
  // if the card is too dense, the row degrades to text only.
  int rowY = y + index * h;

  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "%g", roundTo2(displayValue));
  const char* unit = m->unit;

  // Text sizes: grow with the card (scale), capped for readability
  float valueSize = (float)imax(11, imin((int)(20 * scale), (h * 2) / 3));
  float labelSize = imax(9.0f, valueSize / 2);
  float unitSize  = imax(8.0f, valueSize / 2 + 1);

  // Label and value group get separate width budgets so they never collide
  int contentWidth = w - 32; // chip insets (2*6) + chip padding (2*10)
  float labelMaxWidth = contentWidth * 55 / 100;
  float groupMaxWidth = contentWidth * 42 / 100;

  fitFontSize(m->name, labelMaxWidth, &labelSize, 8);

  float groupWidth = measureText(valueBuf, valueSize).x + 4 +
                     measureText(unit, unitSize).x;
  while (groupWidth > groupMaxWidth && valueSize > 8) {
    valueSize -= 1.0f;
    unitSize = imax(8.0f, valueSize / 2 + 1);
    groupWidth = measureText(valueBuf, valueSize).x + 4 +
                 measureText(unit, unitSize).x;
  }

  // Metric chip: an inset sub-card that groups this metric's data
  int chipX = x + 6, chipY = rowY + 3, chipW = w - 12, chipH = h - 6;
  DrawRectangleRounded((Rectangle){ (float)chipX, (float)chipY,
                                    (float)chipW, (float)chipH },
                       0.15f, 6, THEME.chip);
  int cx = chipX + 10, cw = chipW - 20; // content area inside the chip

  // --- Vertical layout: text line first, sparkline gets the leftover ---
  int textLineH = (int)valueSize + 6;
  bool sparkWanted = g_settings.showSparklines && hist != NULL;
  int sparkH = sparkWanted ? imin(h - textLineH - 6, (int)(26 * scale)) : 0;
  bool showSpark = sparkH >= 10;

  int groupH = textLineH + (showSpark ? sparkH + 6 : 0);
  int groupY = rowY + (h - groupH) / 2;

  drawLabelValueLine(m->name, valueBuf, unit, cx, groupY, cw, textLineH,
                     10, valueSize, labelSize, unitSize, dir, dirAlpha);

  // Sparkline: full chip width, directly under its metric
  if (showSpark) {
    drawSparkline(hist, metricIdx,
                  cx, groupY + textLineH + 6,
                  cw, sparkH);
  }
}


static void drawSensorCard(const sensorData* d, size_t sensorIdx,
                           const sensorHistory* hist, int x, int y, int w, int h,
                           float scale) {
  DrawRectangleRounded((Rectangle){ (float)x, (float)y, (float)w, (float)h },
                       0.12f, 8, THEME.panel);

  // --- Header: sensor name, id on the right ---
  float headerSize = (float)imax(10, imin(16 * scale, h / 5));
  char headerBuf[SENSOR_NAME_MAX];
  snprintf(headerBuf, sizeof(headerBuf), "%s",
           d->name[0] != '\0' ? d->name : "sensor");
  char idBuf[16];
  snprintf(idBuf, sizeof(idBuf), "#%d", d->id);

  float headerMaxWidth = (float)(w - 2 * THEME.padding) -
                         measureText(idBuf, headerSize).x - 8;
  fitFontSize(headerBuf, headerMaxWidth, &headerSize, 9);

  int headerY = y + THEME.padding / 2 + 4;
  drawText(headerBuf, x + THEME.padding, headerY, headerSize, THEME.titleText);

  float idSize = imax(9.0f, headerSize * 3 / 4);
  int idX = x + w - THEME.padding - (int)measureText(idBuf, idSize).x;
  drawText(idBuf, idX, headerY + (int)(headerSize - idSize), idSize,
           THEME.labelText);

  int headerBottom = headerY + (int)headerSize + 6;
  DrawLine(x + THEME.padding, headerBottom,
           x + w - THEME.padding, headerBottom, THEME.divider);

  // --- Metrics ---
  int metricsTop = headerBottom + 6;
  int metricsHeight = (y + h) - metricsTop - THEME.padding / 2;
  if (d->metricCount == 0 || metricsHeight <= 0) return;

  int rowHeight = metricsHeight / (int)d->metricCount;
  if (rowHeight <= 0) return;

  for (size_t i = 0; i < d->metricCount; i++) {
    size_t idx = sensorIdx * MAX_METRICS + i;
    float dirAlpha = 0.0f;
    if (g_dirTimer != NULL && g_dirTimer[idx] > 0.0f && THEME.arrowHoldSeconds > 0.0f) {
      dirAlpha = g_dirTimer[idx] / THEME.arrowHoldSeconds;
    }
    drawMetricRow(&d->metrics[i], x, metricsTop, w, rowHeight, (int)i,
                  g_display != NULL ? g_display[idx] : d->metrics[i].value,
                  g_dir != NULL ? g_dir[idx] : 0, dirAlpha,
                  hist, i, scale);
  }
}

// Draws a gear silhouette (triangle fan) with a punched center hole.
static void drawCogwheel(Vector2 center, float radius, Color color) {
  const int teeth = 8;
  const float rootRatio = 0.72f; // valley radius as fraction of outer
  const float duty = 0.55f;      // fraction of each cycle spent at outer radius

  enum { MAX_OUTLINE = 256 };
  Vector2 points[MAX_OUTLINE + 2];
  int count = 0;
  points[count++] = center; // triangle fan center

  float stepDegrees = 360.0f / (float)teeth;
  for (float a = 0.0f; a < 360.0f; a += 1.5f) {
    if (count >= MAX_OUTLINE) break;
    float phase = fmodf(a, stepDegrees) / stepDegrees;
    float r = (phase < duty) ? radius : radius * rootRatio;
    float rad = a * DEG2RAD;
    // y negated: screen y grows downward, raylib wants counter-clockwise
    points[count++] = (Vector2){ center.x + cosf(rad) * r,
                                 center.y - sinf(rad) * r };
  }
  points[count++] = points[1]; // close the outline

  DrawTriangleFan(points, count, color);
  DrawCircleV(center, radius * 0.35f, THEME.background); // center hole
}

// Draws an on/off pill switch; returns true when clicked.
static bool drawTogglePill(bool value, int x, int y) {
  const float pillW = 34, pillH = 16;
  Rectangle pill = { (float)x, (float)y, pillW, pillH };

  Color track = value ? THEME.valueText : THEME.divider;
  DrawRectangleRounded(pill, 0.6f, 6, track);

  float knobR = pillH / 2.0f - 2.0f;
  float knobX = value ? x + pillW - knobR - 2.0f : x + knobR + 2.0f;
  DrawCircleV((Vector2){ knobX, y + pillH / 2.0f }, knobR, THEME.titleText);

  return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
         CheckCollisionPointRec(GetMousePosition(), pill);
}

// Draws one settings row (label + pill); returns true when clicked.
static bool drawSettingsRow(const char* label, bool* value,
                            Rectangle row) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, row);
  if (hover) DrawRectangleRec(row, THEME.panelHoverColor);

  float labelSize = 13.0f;
  int labelY = (int)row.y + ((int)row.height - (int)labelSize) / 2;
  drawText(label, (int)row.x + THEME.padding, labelY, labelSize,
           THEME.titleText);

  float pillH = 16.0f;
  float pillY = row.y + (row.height - pillH) / 2.0f;
  bool clicked = drawTogglePill(*value,
                                (int)(row.x + row.width - THEME.padding - 34),
                                (int)pillY);
  if (clicked) *value = !*value;
  return clicked;
}

static void drawThemeSwatcher(Rectangle area) {
  const int n = THEME_PRESET_COUNT;
  const float size = 30.0f;
  const float gap = 14.0f;

  float totalW = (float)n * size + (float)(n-1) * gap;
  float x0 = area.x + (area.width - totalW) / 2.0f;
  float y  = area.y + (area.height - size) / 2.0f;
  
  Vector2 mouse = GetMousePosition();
  bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  
  for (int i = 0; i < n; i++) {
      Rectangle sw = {x0 + (size + gap) * (float)i, y, size, size};
      DrawRectangleRounded(sw, 0.25f, 6, THEME_PRESETS[i].valueText);

      if (i == g_settings.theme) {
        Rectangle ring = {sw.x - 3, sw.y - 3, size + 6, size + 6};
        DrawRectangleRoundedLines(ring, 0.25f, 6, THEME.titleText);
      }

      if (pressed && CheckCollisionPointRec(mouse,sw)) {
        ApplyThemePreset(i);
        SaveSettings(AETHER_SETTINGS_FILE, &g_settings);
      }
  }
}

static void drawSettingsPanel(int screenWidth, int screenHeight) {
  const float panelW = 320.0f;
  const float rowH = 36.0f, headerH = 44.0f;
  const float panelH = headerH + 3 * rowH + 26.0f + 40.0f + 12.0f;

  // Dim overlay: pushes the dashboard back, modal-style
  DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 150 });

  Rectangle panel = { (screenWidth - panelW) / 2.0f,
                      (screenHeight - panelH) / 2.0f,
                      panelW, panelH };
  DrawRectangleRounded(panel, 0.06f, 8, THEME.panel);

  drawText("SETTINGS", (int)panel.x + THEME.padding,
           (int)panel.y + 14, 15, THEME.titleText);
  DrawLine((int)panel.x + THEME.padding, (int)(panel.y + headerH) - 6,
           (int)(panel.x + panelW - THEME.padding), (int)(panel.y + headerH) - 6,
           THEME.divider);

  Rectangle row = { panel.x + 4, panel.y + headerH, panelW - 8, rowH };
  bool changed = false;
  changed |= drawSettingsRow("Trend lines", &g_settings.showSparklines, row);
  row.y += rowH;
  changed |= drawSettingsRow("Animated values", &g_settings.animateValues, row);
  row.y += rowH;
  changed |= drawSettingsRow("Change indicators", &g_settings.showIndicators, row);
  row.y += rowH;
  if (changed) SaveSettings(AETHER_SETTINGS_FILE, &g_settings);
  DrawLine((int)panel.x + THEME.padding, (int)row.y,
           (int)(panel.x + panelW - THEME.padding), (int)row.y,
           THEME.divider);

  drawText("Color theme", (int)row.x + THEME.padding,
           (int)row.y + 6, 13, THEME.titleText);

  drawThemeSwatcher((Rectangle){ panel.x, row.y + 26.0f, panelW, 40.0f });
  // remember the panel rect for the outside-click check
}

// Builds the visible tab sequence with collapsing:
// first + last always, current +-1, "..." markers (-1) in larger gaps.
// Returns how many slots were filled (<= 7).
static int buildPageList(int pageCount, int current, int* out, int outMax) {
  return layoutPageList(pageCount, current, out, outMax);
}

// Draws the bottom tab pager; returns true when a tab was clicked.
static bool drawTabPager(int pageCount, int screenWidth, int screenHeight) {
  const float circleD = 24.0f;
  const float gap = 10.0f;
  const float dotsW = 20.0f; // width of a "..." slot

  int seq[9];
  int n = buildPageList(pageCount, g_currentPage + 1, seq, 9);

  // total width to center the strip
  float total = 0.0f;
  for (int i = 0; i < n; i++) total += (seq[i] == -1) ? dotsW : circleD;
  total += gap * (float)(n - 1);

  float x = ((float)screenWidth - total) / 2.0f;
  float cy = (float)screenHeight - THEME.bottomBarHeight / 2.0f;
  Vector2 mouse = GetMousePosition();
  bool clicked = false;

  for (int i = 0; i < n; i++) {
    if (seq[i] == -1) {
      drawText("...", (int)(x + dotsW / 2 - 6), (int)(cy - 7), 13,
               THEME.labelText);
      x += dotsW + gap;
      continue;
    }

    int page = seq[i]; // 1-based
    bool active = page == g_currentPage + 1;
    Vector2 center = { x + circleD / 2.0f, cy };
    bool hover = !active && CheckCollisionPointCircle(mouse, center, circleD / 2.0f + 3.0f);

    Color fill = active ? THEME.valueText
                        : (hover ? THEME.panelHoverColor : THEME.panel);
    Color number = active ? THEME.background
                          : (hover ? THEME.titleText : THEME.labelText);
    DrawCircleV(center, circleD / 2.0f, fill);

    char num[8];
    snprintf(num, sizeof(num), "%d", page);
    Vector2 bounds = measureText(num, 12);
    drawText(num, (int)(center.x - bounds.x / 2.0f),
             (int)(center.y - bounds.y / 2.0f), 12, number);

    if (!active && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      g_currentPage = page - 1;
      clicked = true;
    }

    x += circleD + gap;
  }
  return clicked;
}

// Draws one metric block inside the detail panel: label/value line,
// a large sparkline, and a min/avg/max stats line.
static void drawDetailMetricBlock(const Metric* m, const sensorHistory* hist,
                                  size_t metricIdx, int x, int y, int w, int h,
                                  float displayValue, signed char dir,
                                  float dirAlpha) {
  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "%g", roundTo2(displayValue));

  float valueSize = 24.0f;
  float labelSize = 14.0f;
  float unitSize  = 15.0f;
  int textLineH = (int)valueSize + 8;
  float statsH = 14.0f;

  // Metric chip: same inset sub-card grouping as in the cards
  int chipX = x + 6, chipY = y + 3, chipW = w - 12, chipH = h - 6;
  DrawRectangleRounded((Rectangle){ (float)chipX, (float)chipY,
                                    (float)chipW, (float)chipH },
                       0.08f, 6, THEME.chip);
  int cx = chipX + 14, cw = chipW - 28;

  // Text line at the top, sparkline in the middle, stats always pinned
  // to the bottom of the chip and centered horizontally.
  int textY = chipY + 10;
  drawLabelValueLine(m->name, valueBuf, m->unit, cx, textY, cw, textLineH,
                     10, valueSize, labelSize, unitSize, dir, dirAlpha);

  int sparkTop = textY + textLineH + 4;
  int statsY = chipY + chipH - (int)statsH - 10;
  float sparkH = (float)(statsY - 6 - sparkTop);
  bool showSpark = sparkH >= 24.0f;
  if (sparkH > 90.0f) sparkH = 90.0f;
  if (showSpark) {
    drawSparkline(hist, metricIdx, cx, sparkTop, cw, (int)sparkH);
  }

  float min, max, avg;
  if (sensorHistoryMetricStats(hist, metricIdx, &min, &max, &avg)) {
    char statsBuf[96];
    snprintf(statsBuf, sizeof(statsBuf),
             "min %g  ·  avg %g  ·  max %g", roundTo2(min), roundTo2(avg),
             roundTo2(max));
    Vector2 bounds = measureText(statsBuf, 12);
    drawText(statsBuf, (int)(chipX + (chipW - bounds.x) / 2.0f), statsY, 12,
             THEME.labelText);
  }
}

static void drawDetailPanel(const sensorData* sensors,
                            const sensorHistory* histories,
                            int screenWidth, int screenHeight) {
  const sensorData* d = &sensors[g_detailSensor];
  const sensorHistory* hist = &histories[g_detailSensor];

  // Dim overlay, then the centered panel
  DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 150 });

  float panelW = (float)imin(640, screenWidth - 2 * THEME.padding);
  float panelH = (float)imin(460, screenHeight - 2 * THEME.padding);
  Rectangle panel = { (screenWidth - panelW) / 2.0f,
                      (screenHeight - panelH) / 2.0f, panelW, panelH };
  DrawRectangleRounded(panel, 0.05f, 8, THEME.panel);

  // Header: sensor name + id
  drawText(d->name[0] != '\0' ? d->name : "sensor",
           (int)panel.x + THEME.padding, (int)panel.y + 14, 18,
           THEME.titleText);
  char idBuf[16];
  snprintf(idBuf, sizeof(idBuf), "#%d", d->id);
  Vector2 idBounds = measureText(idBuf, 14);
  drawText(idBuf,
           (int)(panel.x + panelW - THEME.padding - idBounds.x),
           (int)panel.y + 17, 14, THEME.labelText);

  int headerBottom = (int)panel.y + 48;
  DrawLine((int)panel.x + THEME.padding, headerBottom,
           (int)(panel.x + panelW - THEME.padding), headerBottom,
           THEME.divider);

  // Metric blocks share the remaining space
  int metricsTop = headerBottom + 8;
  int metricsH = (int)(panel.y + panelH) - metricsTop - THEME.padding / 2;
  if (d->metricCount == 0 || metricsH <= 0) return;
  int blockH = metricsH / (int)d->metricCount;
  if (blockH <= 0) return;

  int blockX = (int)panel.x + THEME.padding;
  int blockW = (int)panelW - 2 * THEME.padding;

  for (size_t i = 0; i < d->metricCount; i++) {
    size_t idx = (size_t)g_detailSensor * MAX_METRICS + i;
    float dirAlpha = 0.0f;
    if (g_dirTimer != NULL && g_dirTimer[idx] > 0.0f && THEME.arrowHoldSeconds > 0.0f) {
      dirAlpha = g_dirTimer[idx] / THEME.arrowHoldSeconds;
    }
    int by = metricsTop + (int)i * blockH;
    drawDetailMetricBlock(&d->metrics[i], hist, i,
                          blockX, by, blockW, blockH,
                          g_display != NULL ? g_display[idx] : d->metrics[i].value,
                          g_dir != NULL ? g_dir[idx] : 0, dirAlpha);
  }
}

void DrawDashboard(const sensorData* sensors, size_t sensorCount,
                   const sensorHistory* histories,
                   int screenWidth, int screenHeight) {
  BeginDrawing();
  float dt = GetFrameTime();
  if (g_display != NULL) updateAnimations(sensors, dt);
  ClearBackground(THEME.background);

  // Top bar
  drawText("AETHER", THEME.padding, 10, 16, THEME.titleText);

  // Settings cogwheel: far top-right, timer sits left of it
  float iconRadius = 9.0f;
  Vector2 iconCenter = { screenWidth - THEME.padding - iconRadius,
                         THEME.topBarHeight / 2.0f };
  Rectangle iconHit = {
    iconCenter.x - iconRadius - 4, iconCenter.y - iconRadius - 4,
    (iconRadius + 4) * 2, (iconRadius + 4) * 2
  };
  bool iconHovered = CheckCollisionPointRec(GetMousePosition(), iconHit);

	// --- Settings panel input: gear click or S key toggles it ---
	Vector2 mouse = GetMousePosition();
	bool toggleSettings = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && iconHovered) ||
	                      IsKeyPressed(KEY_S);
	if (toggleSettings) {
		g_settingsOpen = !g_settingsOpen;
		g_detailSensor = -1; // the two modals are mutually exclusive
	}

	// Modals close only via ESC
	if (IsKeyPressed(KEY_ESCAPE)) {
		if (g_detailSensor >= 0) g_detailSensor = -1;
		else if (g_settingsOpen) g_settingsOpen = false;
	}

  const char* timeText = TextFormat("%.1f s", GetTime());
  Vector2 timeBounds = measureText(timeText, 14);
  drawText(timeText,
           (int)iconCenter.x - iconRadius - 12 - (int)timeBounds.x, 12, 14,
           THEME.labelText);

  drawCogwheel(iconCenter, iconRadius,
               iconHovered ? THEME.iconHoverColor : THEME.iconColor);

  size_t count = sensorCount;
  if (count == 0) {
    drawCenteredInRow("No sensors loaded", 0, THEME.topBarHeight, screenWidth,
                      screenHeight - THEME.topBarHeight, 20, THEME.labelText);
    if (g_settingsOpen) drawSettingsPanel(screenWidth, screenHeight);
    EndDrawing();
    return;
  }

  // Tab layout: fit as many minimum-size cards as possible; overflow
  // goes to the next tab (page).
  int availW = screenWidth - 2 * THEME.padding;
  int availH = screenHeight - THEME.topBarHeight - THEME.padding;

  TabLayout tabs = layoutTabs(availW, availH,
                              THEME.minCardWidth, THEME.minCardHeight, count);
  int cols = tabs.cols;
  int rows = tabs.rows;
  int perTab = tabs.perTab;
  int pageCount = tabs.pageCount;

  if (g_currentPage >= pageCount) g_currentPage = pageCount - 1;
  if (g_currentPage < 0) g_currentPage = 0;

  // Keyboard: Alt/Cmd + 1..9 jumps to a tab
  bool modifierDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT) ||
                      IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
  if (modifierDown) {
    const int digitKeys[] = { KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
                              KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE };
    for (int k = 0; k < 9; k++) {
      if (IsKeyPressed(digitKeys[k]) && k < pageCount) g_currentPage = k;
    }
  }
  // Arrow keys flip pages
  if (IsKeyPressed(KEY_RIGHT)) g_currentPage++;
  if (IsKeyPressed(KEY_LEFT)) g_currentPage--;
  g_currentPage = imin(imax(g_currentPage, 0), pageCount - 1);

  availH -= tabs.bottomReserved;
  int cardWidth = availW / cols;
  int cardHeight = availH / rows;
  if (cardWidth <= 0 || cardHeight <= 0) {
    EndDrawing();
    return;
  }

  int start = g_currentPage * perTab;
  int end = imin((int)count, start + perTab);
  // Typography grows with card size (reference: ~220px tall cards)
  float scale = imax(100, imin(200, (cardHeight * 100) / 220)) / 100.0f;

  for (int i = start; i < end; i++) {
    int col = (i - start) % cols;
    int row = (i - start) / cols;
    int x = THEME.padding + col * (cardWidth + THEME.cardGap);
    int y = THEME.topBarHeight + row * (cardHeight + THEME.cardGap);
    drawSensorCard(&sensors[i], i,
                   histories != NULL ? &histories[i] : NULL,
                   x, y, cardWidth, cardHeight, scale);

    // Click a card (when no modal is open) to open its detail view
    if (!g_settingsOpen && g_detailSensor < 0 &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mouse, (Rectangle){ (float)x, (float)y,
                                                   (float)cardWidth,
                                                   (float)cardHeight })) {
      g_detailSensor = (int)i;
    }
  }

  // Tab pager (only when there is more than one tab)
  if (tabs.pagerVisible) drawTabPager(pageCount, screenWidth, screenHeight);

  // Detail modal, then settings modal on top of everything
  if (g_detailSensor >= 0)
    drawDetailPanel(sensors, histories, screenWidth, screenHeight);
  if (g_settingsOpen) drawSettingsPanel(screenWidth, screenHeight);

  EndDrawing();
}
