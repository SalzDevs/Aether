#include "dashboard.h"
#include "theme.h"
#include "raylib.h"
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

static void drawMetricRow(const Metric* m, int x, int y, int w, int h, int index,
                          float displayValue, signed char dir, float dirAlpha) {
  int rowY = y + index * h;

  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "%g", roundTo2(displayValue));
  const char* unit = m->unit;

  // Label and value group get separate width budgets so they never collide
  int contentWidth = w - 2 * THEME.padding;
  float labelMaxWidth = contentWidth * 42 / 100;
  float groupMaxWidth = contentWidth * 52 / 100;

  // Sizes: value is the star, label and unit support it
  float valueSize = (float)imax(11, imin(h - 8, imin(h, w) / 3));
  float labelSize = imax(9.0f, valueSize / 2);
  float unitSize  = imax(8.0f, valueSize / 2 + 1);

  fitFontSize(m->name, labelMaxWidth, &labelSize, 8);

  float groupWidth = measureText(valueBuf, valueSize).x + 4 +
                     measureText(unit, unitSize).x;
  while (groupWidth > groupMaxWidth && valueSize > 8) {
    valueSize -= 1.0f;
    unitSize = imax(8.0f, valueSize / 2 + 1);
    groupWidth = measureText(valueBuf, valueSize).x + 4 +
                 measureText(unit, unitSize).x;
  }

  // Label on the left, vertically centered
  int labelY = rowY + (h - (int)labelSize) / 2;
  drawText(m->name, x + THEME.padding, labelY, labelSize, THEME.labelText);

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

  // Value + unit on the right, vertically centered as a group
  int groupX = x + w - THEME.padding - (int)groupWidth;
  int valueY = rowY + (h - (int)valueSize) / 2;

  if (arrowWidth > 0) {
    float ax = (float)(groupX - arrowWidth);
    float ay = rowY + h / 2.0f;
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
  int unitY = rowY + (h - (int)unitSize) / 2 + (int)(valueSize - unitSize) / 3;
  drawText(unit, unitX, unitY, unitSize, THEME.labelText);
}


static void drawSensorCard(const sensorData* d, size_t sensorIdx, int x, int y, int w, int h) {
  DrawRectangleRounded((Rectangle){ (float)x, (float)y, (float)w, (float)h },
                       0.12f, 8, THEME.panel);

  // --- Header: sensor name, id on the right ---
  float headerSize = (float)imax(10, imin(16, h / 5));
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
                  g_dir != NULL ? g_dir[idx] : 0, dirAlpha);
  }
}

void DrawDashboard(const sensorData* sensors, size_t sensorCount,
                   int screenWidth, int screenHeight) {
  BeginDrawing();
  float dt = GetFrameTime();
  if (g_display != NULL) updateAnimations(sensors, dt);
  ClearBackground(THEME.background);

  // Top bar
  drawText("AETHER", THEME.padding, 10, 16, THEME.titleText);
  const char* timeText = TextFormat("%.1f s", GetTime());
  Vector2 timeBounds = measureText(timeText, 14);
  drawText(timeText,
           screenWidth - THEME.padding - (int)timeBounds.x, 12, 14,
           THEME.labelText);

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
    drawSensorCard(&sensors[i], i, x, y, cardWidth, cardHeight);
  }

  EndDrawing();
}
