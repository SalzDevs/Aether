#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../Queue/queue.h"

// Loads the dashboard font and other GPU resources.
// Must be called after InitWindow and before DrawDashboard.
void InitDashboard(void);

// Releases dashboard resources. Call before CloseWindow.
void UnloadDashboard(void);

// Draws one frame of the dashboard: clears the screen, renders the
// top bar and one card per sensor currently in the queue.
// Must be called once per frame while the window is open.
void DrawDashboard(const sensorQueue* sq, int screenWidth, int screenHeight);

#endif
