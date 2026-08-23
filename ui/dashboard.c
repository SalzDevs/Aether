#include "dashboard.h"
#include "raylib.h"
#include <stdio.h>

// Palette
static const Color BACKGROUND_COLOR = { 24, 26, 32, 255 };
static const Color PANEL_COLOR      = { 33, 37, 47, 255 };
static const Color TITLE_TEXT_COLOR = { 235, 240, 250, 255 };
static const Color LABEL_TEXT_COLOR = { 145, 152, 168, 255 };
static const Color VALUE_TEXT_COLOR = { 110, 190, 255, 255 };
static const Color DIVIDER_COLOR    = { 52, 57, 70, 255 };

// Layout
static const int CONTENT_PADDING = 16;
static const int CARD_GAP        = 12;
static const int TOPBAR_HEIGHT   = 34;

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

// Shrinks fontSize until text fits maxWidth (or hits the floor).
static void fitFontSize(const char* text, int maxWidth, int* fontSize, int floorSize) {
  int width = MeasureText(text, *fontSize);
  while (width > maxWidth && *fontSize > floorSize) {
    (*fontSize)--;
    width = MeasureText(text, *fontSize);
  }
}

// Draws a single centered line of text inside the given row.
static void drawCenteredInRow(const char* text, int x, int y, int w, int h,
                              int fontSize, Color color) {
  int posX = x + (w - MeasureText(text, fontSize)) / 2;
  int posY = y + (h - fontSize) / 2;
  DrawText(text, posX, posY, fontSize, color);
}

static void drawMetricRow(const Metric* m, int x, int y, int w, int h, int index) {
  int rowY = y + index * h;

  char valueBuf[32];
  snprintf(valueBuf, sizeof(valueBuf), "%g", m->value);
  const char* unit = m->unit;

  // Label and value group get separate width budgets so they never collide
  int contentWidth = w - 2 * CONTENT_PADDING;
  int labelMaxWidth = contentWidth * 42 / 100;
  int groupMaxWidth = contentWidth * 52 / 100;

  // Sizes: value is the star, label and unit support it
  int valueSize = imax(11, imin(h - 8, imin(h, w) / 3));
  int labelSize = imax(9, valueSize / 2);
  int unitSize  = imax(8, valueSize / 2 + 1);

  fitFontSize(m->name, labelMaxWidth, &labelSize, 8);

  int groupWidth = MeasureText(valueBuf, valueSize) + 4 +
                   MeasureText(unit, unitSize);
  while (groupWidth > groupMaxWidth && valueSize > 8) {
    valueSize--;
    unitSize = imax(8, valueSize / 2 + 1);
    groupWidth = MeasureText(valueBuf, valueSize) + 4 +
                 MeasureText(unit, unitSize);
  }

  // Label on the left, vertically centered
  int labelY = rowY + (h - labelSize) / 2;
  DrawText(m->name, x + CONTENT_PADDING, labelY, labelSize, LABEL_TEXT_COLOR);

  // Value + unit on the right, vertically centered as a group
  int groupX = x + w - CONTENT_PADDING - groupWidth;
  int valueY = rowY + (h - valueSize) / 2;
  DrawText(valueBuf, groupX, valueY, valueSize, VALUE_TEXT_COLOR);
  int unitX = groupX + MeasureText(valueBuf, valueSize) + 4;
  int unitY = rowY + (h - unitSize) / 2 + (valueSize - unitSize) / 3;
  DrawText(unit, unitX, unitY, unitSize, LABEL_TEXT_COLOR);
}

static void drawSensorCard(const sensorData* d, int x, int y, int w, int h) {
  DrawRectangleRounded((Rectangle){ (float)x, (float)y, (float)w, (float)h },
                       0.12f, 8, PANEL_COLOR);

  // --- Header: sensor name, id on the right ---
  int headerSize = imax(10, imin(16, h / 5));
  char headerBuf[SENSOR_NAME_MAX];
  snprintf(headerBuf, sizeof(headerBuf), "%s",
           d->name[0] != '\0' ? d->name : "sensor");
  char idBuf[16];
  snprintf(idBuf, sizeof(idBuf), "#%d", d->id);

  int headerMaxWidth = w - 2 * CONTENT_PADDING - MeasureText(idBuf, headerSize) - 8;
  fitFontSize(headerBuf, headerMaxWidth, &headerSize, 9);

  int headerY = y + CONTENT_PADDING / 2 + 4;
  DrawText(headerBuf, x + CONTENT_PADDING, headerY, headerSize, TITLE_TEXT_COLOR);

  int idSize = imax(9, headerSize * 3 / 4);
  DrawText(idBuf,
           x + w - CONTENT_PADDING - MeasureText(idBuf, idSize),
           headerY + (headerSize - idSize), idSize, LABEL_TEXT_COLOR);

  int headerBottom = headerY + headerSize + 6;
  DrawLine(x + CONTENT_PADDING, headerBottom,
           x + w - CONTENT_PADDING, headerBottom, DIVIDER_COLOR);

  // --- Metrics ---
  int metricsTop = headerBottom + 6;
  int metricsHeight = (y + h) - metricsTop - CONTENT_PADDING / 2;
  if (d->metricCount == 0 || metricsHeight <= 0) return;

  int rowHeight = metricsHeight / (int)d->metricCount;
  if (rowHeight <= 0) return;

  for (size_t i = 0; i < d->metricCount; i++) {
    drawMetricRow(&d->metrics[i], x, metricsTop, w, rowHeight, (int)i);
  }
}

void DrawDashboard(const sensorQueue* sq, int screenWidth, int screenHeight) {
  BeginDrawing();
  ClearBackground(BACKGROUND_COLOR);

  // Top bar
  DrawText("AETHER", CONTENT_PADDING, 10, 16, TITLE_TEXT_COLOR);
  const char* timeText = TextFormat("%.1f s", GetTime());
  DrawText(timeText,
           screenWidth - CONTENT_PADDING - MeasureText(timeText, 14),
           12, 14, LABEL_TEXT_COLOR);

  size_t count = sq->current_size;
  if (count == 0) {
    drawCenteredInRow("No sensors loaded", 0, TOPBAR_HEIGHT, screenWidth,
                      screenHeight - TOPBAR_HEIGHT, 20, LABEL_TEXT_COLOR);
    EndDrawing();
    return;
  }

  // Adaptive grid: roughly square layout (cols ~= sqrt(count))
  int cols = 1;
  while (cols * cols < (int)count) cols++;
  int rows = ((int)count + cols - 1) / cols;

  int availW = screenWidth - 2 * CONTENT_PADDING - (cols - 1) * CARD_GAP;
  int availH = screenHeight - TOPBAR_HEIGHT - CONTENT_PADDING - (rows - 1) * CARD_GAP;
  int cardWidth = availW / cols;
  int cardHeight = availH / rows;
  if (cardWidth <= 0 || cardHeight <= 0) {
    EndDrawing();
    return;
  }

  for (size_t i = 0; i < count; i++) {
    int col = (int)i % cols;
    int row = (int)i / cols;
    int x = CONTENT_PADDING + col * (cardWidth + CARD_GAP);
    int y = TOPBAR_HEIGHT + row * (cardHeight + CARD_GAP);
    drawSensorCard(&sq->data[i], x, y, cardWidth, cardHeight);
  }

  EndDrawing();
}
