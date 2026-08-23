#include "dashboard.h"
#include "raylib.h"
#include <stdio.h>

// Palette
static const Color BACKGROUND_COLOR = { 24, 26, 32, 255 };
static const Color TIMER_TEXT_COLOR = { 150, 158, 172, 255 };
static const Color DATA_TEXT_COLOR  = { 110, 190, 255, 255 };
static const Color DIVIDER_COLOR    = { 52, 57, 70, 255 };

// Layout
static const int CONTENT_PADDING = 20;

static int displayBlockHeightCalc(int screenHeight, int amountOfSensors) {
  return screenHeight / amountOfSensors;
}

static void shrinkFontSize(const char* buf, int *fontSize, int maxWidth, int *textWidth) {
  if (!fontSize || !textWidth) return;

  *textWidth = MeasureText(buf, *fontSize);

  while ((*textWidth > maxWidth) && (*fontSize > 1)) {
    (*fontSize)--;
    *textWidth = MeasureText(buf, *fontSize);
  }
}

void DrawDashboard(const sensorQueue* sq, int screenWidth, int screenHeight) {
  BeginDrawing();
  ClearBackground(BACKGROUND_COLOR);

  DrawText(TextFormat("Time: %.02f", GetTime()), 0, 0, 10, TIMER_TEXT_COLOR);

  int padding = CONTENT_PADDING;
  int maxWidth = screenWidth - (padding * 2);
  int displayBlockHeight = displayBlockHeightCalc(screenHeight, (int)sq->current_size);

  char buf[256];
  int textWidth;

  for (size_t i = 0; i < sq->current_size; i++) {
    sensorDataToString(sq->data[i], buf, sizeof(buf));

    int fontSize = displayBlockHeight - 10;

    shrinkFontSize(buf, &fontSize, maxWidth, &textWidth);

    int posX = (screenWidth - textWidth) / 2;
    int posY = (displayBlockHeight * i) + (displayBlockHeight - fontSize) / 2;

    DrawText(buf, posX, posY, fontSize, DATA_TEXT_COLOR);
    DrawLine(0, displayBlockHeight * i, screenWidth, displayBlockHeight * i, DIVIDER_COLOR);
  }

  EndDrawing();
}
