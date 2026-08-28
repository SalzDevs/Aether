#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../sensor/sensor.h"
#include "../History/history.h"
#include <stddef.h>

// Loads the dashboard font and animation state.
// Must be called after InitWindow and before DrawDashboard.
// The registry is used to seed displayed values.
void InitDashboard(const sensorData* sensors, size_t sensorCount);

// Loads persisted settings from the given file and applies them (theme + toggles).
// Falls back to defaults when the file is missing or invalid.
void InitSettings(const char* path);

// Releases dashboard resources. Call before CloseWindow.
void UnloadDashboard(void);

// Draws one frame of the dashboard: clears the screen, renders the
// top bar and one card per sensor in the registry. Each metric row
// includes a sparkline built from that sensor's reading history.
// Must be called once per frame while the window is open.
void DrawDashboard(const sensorData* sensors, size_t sensorCount,
                   const sensorHistory* histories,
                   int screenWidth, int screenHeight);

#endif
