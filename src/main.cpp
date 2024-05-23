#include <Arduino.h>

#include <Wire.h>
#include <FS.h>
#include "SPIFFS.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include "OV7670.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "BMP.h"

const char* ssid = "GRTenda-24";
const char* password = "robi2002";

const int TFT_DC = 2;
const int TFT_CS = 5;
const int TFT_SDA = 23;
const int TFT_SCK = 18;
const int TFT_RST = 17;

const int D0 = 13;
const int D1 = 12;
const int D2 = 14;
const int D3 = 27;
const int D4 = 26;
const int D5 = 25;
const int D6 = 33;
const int D7 = 32;

const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int VSYNC = 34;
const int HREF = 35;

const int XCLK = 16;
const int PCLK = 4;

#define ROTATION 3
// Screen dimensions
#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 128

#define RADIUS 40

// Center of the speedometer
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)

WiFiMulti wifiMulti;
WiFiServer server(80);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
OV7670 *camera;

unsigned char bmpHeader[BMP::headerSize];

void serve()
{
  WiFiClient client = server.available();
  if (client) 
  {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(
              "<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
        
        if (currentLine.endsWith("GET /camera")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:image/bmp");
          client.println();
          
          client.write(bmpHeader, BMP::headerSize);
          client.write(camera->frame, camera->xres * camera->yres * 2);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }  
}

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

void setup()
{
  Serial.begin(115200);

  wifiMulti.addAP(ssid, password);
  //wifiMulti.addAP(ssid2, password2);
  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  
  camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  Serial.println("Created camera");

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(0);
  Serial.print(tft.width());
  Serial.print(" x ");
  Serial.println(tft.height());
  server.begin();
  Serial.println("Started server");
  drawSpeedometer();
}

void displayY8(unsigned char * frame, int xres, int yres)
{
  tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  int i = 0;
  for (int x = 0; x < xres; x++)
    for (int y = 0; y < yres; y++) {
      i = y * xres + x;
      unsigned char c = frame[i];
      unsigned short r = c >> 3;
      unsigned short g = c >> 2;
      unsigned short b = c >> 3;
      tft.pushColor(r << 11 | g << 5 | b);
    }  
}

void displayRGB565(unsigned char * frame, int xres, int yres)
{
  tft.fillScreen(0);
  tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  int i = 0;
  for (int x = 0; x < xres; x++)
    for (int y = 0; y < yres; y++) {
      i = (y * xres + x) << 1;
      tft.pushColor((frame[i] | (frame[i+1] << 8)));
    }
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

static int counter;

void loop()
{
  // camera->oneFrame();
  // serve();
  // displayRGB565(camera->frame, camera->xres, camera->yres);

  // tft.fillScreen(0);
  // tft.drawCircle(tft.width() / 2, tft.height() / 2, 40, 0x001F);
  // tft.drawLine(tft.width() / 2, tft.height() / 2, , , 0xFFFF);
  // tft.setCursor(tft.width() / 2, tft.height() / 2);
  // tft.println(counter++);
  // tft.setCursor(tft.width() / 2, tft.height() / 2 + 10);
  // tft.println("km/h");
  drawCursor(counter++);
  drawSpeed(counter);
  // sleep(1);
  usleep(500 * 1000);
}
