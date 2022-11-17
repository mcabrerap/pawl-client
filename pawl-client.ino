#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include <Arduino.h>

#include <ArduinoJson.h>

#define ONE_SECOND 1000
#define TEN_SECONDS 10000

char ssid[] = "Google Home Wifi";
char password[] = "M4idy2831";

void setup() {

  Serial.begin(9600);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);

    delay(ONE_SECOND);
  }

  Serial.print("You're connected to the network");
}

void loop() {

  String command = fecthCommand();

  Serial.println(command); 
}

String fecthCommand() {

  String command = "";

  if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;

    http.begin("http://192.168.0.7:3000/pawl/v1/api/command/");
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();

    if (httpCode == 200) {

      command = http.getString();            
    }

    http.end();

    delay(TEN_SECONDS);   
  }

  return command;
}
