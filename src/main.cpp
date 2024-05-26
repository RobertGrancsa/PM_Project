#include <Arduino.h>

#include <Wire.h>
#include <FS.h>
#include "SPIFFS.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include "OV7670.h"
#include "speedometer.h"
#include "data_send.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <Ticker.h>
#include "BMP.h"

#include <TinyGPS++.h>

const char* ssid = "GRTenda-24";
const char* password = "robi2002";

const int TFT_DC = 2;
const int TFT_CS = 5;
const int TFT_SDA = 23;
const int TFT_SCK = 18;
const int TFT_RST = 15;

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

const int XCLK = 19;
const int PCLK = 4;

const int HALL = 39;
const int BTN = 15;

#define ROTATION 3

// Variables to store time and speed
volatile unsigned long lastInterruptTime = 0;
volatile unsigned long currentInterruptTime = 0;
volatile float wheelSpeed = 0.0;
volatile float totalDistance = 0.0;
volatile unsigned long interruptCount = 0;

// Wheel diameter in meters (example: 0.7 meters for a typical bike wheel)
const float wheelDiameter = 0.3;
const float wheelCircumference = PI * wheelDiameter;

WiFiMulti wifiMulti;
WiFiServer server(80);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
OV7670 *camera;

TinyGPSPlus gps;

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

Ticker timer_update;
Ticker timer_upload;
// Timer handles
hw_timer_t* timer1 = NULL;
hw_timer_t* timer2 = NULL;

TaskHandle_t timer_upload_task;
TaskHandle_t timer_update_task;

void attach_timer(void *parameter) {
  Serial.print("Creating send task on ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    send_data();
    Serial.println("Task1: Executing on core 0");
    vTaskDelay(5000);
  }
}

// ISR for Hall effect sensor
void onHallEffect() {
  Serial.println("Found signal change");
  currentInterruptTime = micros();
  unsigned long timeDiff = currentInterruptTime - lastInterruptTime;

  wheelSpeed = wheelCircumference / (timeDiff / 1000000.0);

  // Update total distance traveled
  totalDistance += wheelCircumference;

  // Update the interrupt count
  interruptCount++;

  lastInterruptTime = currentInterruptTime;
  updateTimerScreen(wheelSpeed, totalDistance);
}


void update_timer(void *parameter) {
  Serial.print("Creating update task on ");
  Serial.println(xPortGetCoreID());
  // timer_update.attach(0.5f, updateTimerScreen);

  bool found = false;
  bool reset = false;
  for (;;) {
    // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.println("Task2: Executing on core 1");
    uint16_t speed = analogRead(HALL);

    if (!found && speed == 0) {
      onHallEffect();
      found = true;
      reset = false;
    } else if (speed == 4095) {
      found = false;
    }

    if (!reset && micros() - lastInterruptTime > 5000) {
      wheelSpeed = 0.0;
      updateTimerScreen(wheelSpeed, totalDistance);
      reset = true;
    }
    vTaskDelay(1);
  }
}

void IRAM_ATTR buttonPressed(void *) {
  Serial.println("Pressed button");
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Started on core ");
  Serial.println(xPortGetCoreID());

  Serial2.begin(9600);
  Serial.println(F("ESP32 - GPS module"));


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

  BaseType_t ret = xTaskCreatePinnedToCore(
      attach_timer, /* Function to implement the task */
      "timer_send_data", /* Name of the task */
      20000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &timer_upload_task,  /* Task handle. */
      0); /* Core where the task should run */

  Serial.println(ret == pdPASS ? "pdPASS" : "errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");

  ret = xTaskCreatePinnedToCore(
      update_timer, /* Function to implement the task */
      "timer_send_data", /* Name of the task */
      20000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &timer_update_task,  /* Task handle. */
      1); /* Core where the task should run */

  int rc = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  if (rc != ESP_OK) {
    Serial.print("Error installing ISR service ");
    Serial.println(rc);
    return;
  }

  // Configure Hall effect sensor pin
  pinMode(BTN, INPUT_PULLUP);  // Use internal pull-up resistor
  if (gpio_isr_handler_add((gpio_num_t)BTN, buttonPressed, (void*) BTN) != ESP_OK) {
    Serial.println("Error attaching ISR handler");
    return;
  }  // Trigger on falling edge
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

void displayBitmap(unsigned char *frame, int xres, int yres)
{
  tft.fillScreen(0);
  tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  tft.drawRGBBitmap(0, 0, (uint16_t *)frame, xres, yres);
}

void loop()
{
  // camera->oneFrame();
  // serve();
  // displayBitmap(camera->frame, camera->xres, camera->yres);

  // tft.fillScreen(0);
  // tft.drawCircle(tft.width() / 2, tft.height() / 2, 40, 0x001F);
  // tft.drawLine(tft.width() / 2, tft.height() / 2, , , 0xFFFF);
  // tft.setCursor(tft.width() / 2, tft.height() / 2);
  // tft.println(counter++);
  // tft.setCursor(tft.width() / 2, tft.height() / 2 + 10);
  // tft.println("km/h");  
  // sleep(1);
  static uint16_t max = 0;
  static uint16_t min = UINT16_MAX;
  uint16_t speed = analogRead(HALL);
  if (speed > max) max = speed;
  if (speed < min) min = speed;

  Serial.print(speed);
  Serial.printf(", min: %hu, max: %hu, signal: %d\n", min, max, digitalRead(HALL));

  Serial.print("Wheel speed: ");
  Serial.print(wheelSpeed);
  Serial.println(" m/s");

  usleep(1000 * 1000);

  if (Serial.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat());

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng());

        Serial.print(F("- altitude: "));
        if (gps.altitude.isValid())
          Serial.println(gps.altitude.meters());
        else
          Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }

      Serial.print(F("- speed: "));
      if (gps.speed.isValid()) {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.println();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}
