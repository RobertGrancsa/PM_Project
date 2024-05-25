#include "data_send.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>

extern WiFiMulti wifiMulti;
extern int counter;
const char* serverName = "http://192.168.1.135:3000/api/log-data";

void send_data() {
  Serial.print("Trying send on core ");
  Serial.println(xPortGetCoreID());
  if (WiFi.status() == WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
  
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    
    // If you need Node-RED/server authentication, insert user and password below
    //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
    // Data to send with HTTP POST
    // Send HTTP POST request
    // int httpResponseCode = http.POST(httpRequestData);
    
    // If you need an HTTP request with a content type: application/json, use the following:
    //http.addHeader("Content-Type", "application/json");
    String data = "{\"speed\":";
    data.concat(counter);
    data.concat(",\"distance_elapsed\":");
    data.concat(counter);
    data.concat("}");

    int httpResponseCode = http.POST(data);

    // If you need an HTTP request with a content type: text/plain
    //http.addHeader("Content-Type", "text/plain");
    //int httpResponseCode = http.POST("Hello, World!");
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
      
    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}