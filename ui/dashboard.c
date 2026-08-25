#include "dashboard.h"
#include "theme.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const DashboardTheme THEME = {
    .background     = { 24, 26, 32, 255 },
    .panel          = { 33, 37, 47, 255 },
    .titleText      = { 235, 240, 250, 255 },
    .labelText      = { 145, 152, 168, 255 },
    .valueText      = { 110, 190, 255, 255 },
    .upColor        = { 130, 205, 140, 255 },
    .downColor      = { 235, 135, 120, 255 },
    .divider        = { 52, 57, 70, 255 },
    .padding        = 16,
    .cardGap        = 12,
    .topBarHeight   = 34,
    .fontAtlasSize  = 64.0f,
    .iconColor      = { 145, 152, 168, 255 },
    .iconHoverColor = { 235, 240, 250, 255 },
    .arrowHoldSeconds = 1.5f,
    .valueSmoothing   = 10.0f,
};

#define FONT_PATH "assets/fonts/JetBrainsMono-Regular.ttf"

static Font g_font;
static bool g_fontLoaded = false;

// --- Animation state (one slot per metric, indexed sensor*MAX_METRICS+metric) ---
static float* g_display = NULL;   // smoothed value currently shown
static float* g_prev = NULL;      // last actual value seen (change detection)
static signed char* g_dir = NULL; // direction of last change: -1, 0, +1
static float* g_dirTimer = NULL;  // seconds left before the arrow fades
static size_t g_sensorCount = 0;

void InitDashboard(const sensorData* sensors, size_t sensorCount) {
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
    g_font = LoadFontEx(FONT_PATH, (int)THEME.fontAtlasSize, NULL, 0);
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

      g_display[idx] += (target - g_display[idx]) * alpha;

      if (target != g_prev[idx]) {
        g_dir[idx] = target > g_prev[idx] ? 1 : -1;
        g_dirTimer[idx] = THEME.arrowHoldSeconds;
        g_prev[idx] = target;
      }
      if (g_dirTimer[idx] > 0.0f) g_dirTimer[idx] -= dt;
    }
  }
}

// Rounds to two decimals so animated values render compactly.
static float roundTo2(float v) {
  long scaled = (long)(v * 100.0f + (v >= 0.0f ? 0.5f : -0.5f));
  return (float)scaled / 100.0f;
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
  float min = 0.0f, max = 0.0f;
  for (size_t i = 0; i < count; i++) {
    const sensorData* reading = sensorHistoryAt(hist, i);
    if (metricIdx >= reading->metricCount) return 0;
    values[i] = reading->metrics[metricIdx].value;
    if (i == 0 || values[i] < min) min = values[i];
    if (i == 0 || values[i] > max) max = values[i];
  }

  // Vertical layout: small margins inside the row
  float top = (float)y + 4.0f;
  float bottom = (float)(y + h) - 4.0f;
  if (bottom - top < 4.0f) return 0;

  Vector2 points[SPARK_MAX_POINTS];
  float step = (float)maxW / (float)(count - 1);
  for (size_t i = 0; i < count; i++) {
    float t = (max > min) ? (values[i] - min) / (max - min) : 0.5f;
    points[i] = (Vector2){ (float)x + step * (float)i,
                           bottom - t * (bottom - top) };
  }

  Color lineColor = { THEME.valueText.r, THEME.valueText.g,
                      THEME.valueText.b, 110 };
  DrawLineStrip(points, (int)count, lineColor);

  // Bright dot on the newest reading
  Vector2 last = points[count - 1];
  DrawCircleV(last, 2.0f, THEME.valueText);

  return maxW;
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
  int contentWidth = w - 2 * THEME.padding;
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

  // Up/down change indicator: small triangle left of the value
  float arrowSize = valueSize / 3.0f;
  int arrowWidth = 0;
  Color arrowColor = THEME.labelText;
  if (dir != 0 && dirAlpha > 0.0f) {
    arrowWidth = (int)arrowSize + 6;
    Color base = dir > 0 ? THEME.upColor : THEME.downColor;
    arrowColor = (Color){ base.r, base.g, base.b,
                          (unsigned char)(255 * dirAlpha) };
  }

  // --- Vertical layout: text line first, sparkline gets the leftover ---
  int textLineH = (int)valueSize + 6;
  int sparkH = imin(h - textLineH - 6, (int)(26 * scale)); // 6px gap, capped
  bool showSpark = sparkH >= 10;

  int groupH = textLineH + (showSpark ? sparkH + 6 : 0);
  int groupY = rowY + (h - groupH) / 2;

  // Text line: label left, value + unit right
  int labelY = groupY + (textLineH - (int)labelSize) / 2;
  int valueY = groupY + (textLineH - (int)valueSize) / 2;
  drawText(m->name, x + THEME.padding, labelY, labelSize, THEME.labelText);

  int groupX = x + w - THEME.padding - (int)groupWidth;
  if (arrowWidth > 0) {
    float ax = (float)(groupX - arrowWidth);
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

  // Sparkline: full content width, directly under its metric
  if (showSpark) {
    drawSparkline(hist, metricIdx,
                  x + THEME.padding, groupY + textLineH + 6,
                  contentWidth, sparkH);
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
    EndDrawing();
    return;
  }

  // Adaptive grid: roughly square layout (cols ~= sqrt(count))
  int cols = 1;
  while (cols * cols < (int)count) cols++;
  int rows = ((int)count + cols - 1) / cols;

  int availW = screenWidth - 2 * THEME.padding - (cols - 1) * THEME.cardGap;
  int availH = screenHeight - THEME.topBarHeight - THEME.padding
               - (rows - 1) * THEME.cardGap;
  int cardWidth = availW / cols;
  int cardHeight = availH / rows;
  if (cardWidth <= 0 || cardHeight <= 0) {
    EndDrawing();
    return;
  }

  for (size_t i = 0; i < count; i++) {
    int col = (int)i % cols;
    int row = (int)i / cols;
    int x = THEME.padding + col * (cardWidth + THEME.cardGap);
    int y = THEME.topBarHeight + row * (cardHeight + THEME.cardGap);
    // Typography grows with card size (reference: ~220px tall cards)
    float scale = imax(100, imin(200, (cardHeight * 100) / 220)) / 100.0f;
    drawSensorCard(&sensors[i], i,
                   histories != NULL ? &histories[i] : NULL,
                   x, y, cardWidth, cardHeight, scale);
  }

  EndDrawing();
}
