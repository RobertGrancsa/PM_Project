#include "data_send.h"

#include "OV7670.h"
#include "mbedtls/base64.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include <TinyGPS++.h>

extern WiFiMulti wifiMulti;
extern OV7670 *camera;
extern TinyGPSPlus gps;

extern float wheelSpeed;
extern float totalDistance;
const char* serverName = "http://192.168.1.135:3000/api/log-data";

const int bufferSize = 320 * 240 * 2;

const int chunkSize = 3;
char base64Output[5];
IPAddress ip(192, 168, 1, 135);

void send_data() {
  Serial.print("Trying send on core ");
  Serial.println(xPortGetCoreID());
  if (WiFi.status() == WL_CONNECTED && camera->frame) {
    WiFiClient client;
    HTTPClient http;
  
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    
    // If you need Node-RED/server authentication, insert user and password below
    //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
    // http.addHeader("Transfer-Encoding", "chunked");
    // WiFiClient* stream = http.getStreamPtr();

    // If you need an HTTP request with a content type: application/json, use the following:
    //http.addHeader("Content-Type", "application/json");
    String data = "{\"speed\":";
    data.concat(wheelSpeed);
    data.concat(",\"distance_elapsed\":");
    data.concat(totalDistance);
    data.concat("}");

    vTaskDelay(1);

    Serial.println(data);
    int httpResponseCode = http.POST(data);

    // Wait for server response
    // while (client.connected()) {
    //   String line = client.readStringUntil('\n');
    //   if (line == "\r") {
    //     break;
    //   }
    // }

    // // Read and print the server response
    // String response = client.readString();
    Serial.print("Response: ");
    Serial.println(httpResponseCode);

    // client.stop();
    vTaskDelay(1);  // Short delay to yield
  } else {
    Serial.println("WiFi Disconnected");
  }
}