#include "speedometer.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library

extern Adafruit_ST7735 tft;

void drawSpeedometer() {
  tft.fillScreen(ST7735_BLACK);
  tft.drawCircle(CENTER_X, CENTER_Y, RADIUS, ST7735_WHITE);
  
  for (int angle = -180; angle <= 0; angle += 10) {
    float radian = angle * DEG_TO_RAD;
    int x0 = CENTER_X + cos(radian) * (RADIUS - 10);
    int y0 = CENTER_Y + sin(radian) * (RADIUS - 10);
    int x1 = CENTER_X + cos(radian) * RADIUS;
    int y1 = CENTER_Y + sin(radian) * RADIUS;
    tft.drawLine(x0, y0, x1, y1, ST7735_WHITE);
  }

  tft.setCursor(CENTER_X - 10, CENTER_Y + 15);
  tft.print("km/h");
}

void drawCursor(int speed) {
  static int lastX = CENTER_X;
  static int lastY = CENTER_Y;

  // Erase the previous cursor
  tft.drawLine(CENTER_X, CENTER_Y, lastX, lastY, ST7735_BLACK);

  // Calculate the new cursor position
  float angle = map(speed, 0, 100, -180, 180);
  float radian = angle * DEG_TO_RAD;
  int x = CENTER_X + cos(radian) * (RADIUS - 20);
  int y = CENTER_Y + sin(radian) * (RADIUS - 20);

  // Draw the new cursor
  tft.drawLine(CENTER_X, CENTER_Y, x, y, ST7735_RED);

  // Update last cursor position
  lastX = x;
  lastY = y;
}

void drawSpeed(int speed) {
  static int lastSpeed = 0;

  tft.setCursor(CENTER_X - 5, CENTER_Y + 5);
  tft.setTextColor(ST7735_BLACK);
  tft.print(lastSpeed);

  tft.setCursor(CENTER_X - 5, CENTER_Y + 5);
  tft.setTextColor(ST7735_WHITE);
  tft.print(speed);

  lastSpeed = speed;
}

int counter;

void updateTimerScreen(float speed) {
  Serial.print("Updating on core ");
  Serial.println(xPortGetCoreID());
  drawCursor(speed * 3.6f);
  drawSpeed(speed * 3.6f);
}