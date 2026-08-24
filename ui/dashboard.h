#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../sensor/sensor.h"
#include <stddef.h>

// Loads the dashboard font and animation state.
// Must be called after InitWindow and before DrawDashboard.
// The registry is used to seed displayed values.
void InitDashboard(const sensorData* sensors, size_t sensorCount);

// Releases dashboard resources. Call before CloseWindow.
void UnloadDashboard(void);

// Draws one frame of the dashboard: clears the screen, renders the
// top bar and one card per sensor in the registry.
// Must be called once per frame while the window is open.
void DrawDashboard(const sensorData* sensors, size_t sensorCount,
                   int screenWidth, int screenHeight);

#endif
