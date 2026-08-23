#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../Queue/queue.h"

// Draws one frame of the dashboard: clears the screen, renders the
// timer and one text block per sensor currently in the queue.
// Must be called once per frame while the window is open.
void DrawDashboard(const sensorQueue* sq, int screenWidth, int screenHeight);

#endif
