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

#define ROTATION 0

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
  tft.fillScreen(0);
  server.begin();
  Serial.println("Started server");
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

void loop()
{
  camera->oneFrame();
  serve();
  displayRGB565(camera->frame, camera->xres, camera->yres);
}
