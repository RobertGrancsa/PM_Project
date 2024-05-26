#pragma once
#include <Arduino.h>

// Screen dimensions
#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 128

#define RADIUS 40

// Center of the speedometer
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)

void drawSpeedometer();
void drawCursor(int speed);
void drawSpeed(int speed);
void updateTimerScreen(float speed, float distance);