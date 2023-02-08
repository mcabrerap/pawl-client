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

#define RXD2 16
#define TXD2 17

char ssid[] = "Google Home Wifi";
char password[] = "M4idy2831";

String apiUrl = "http://192.168.0.8:3000";

String voltageHeader = "{\"voltage\":\"";
String currentHeader = "\",\"current\":\"";
String identifierHeader = "\",\"identifier\":\"";
String deviceIdHeader = "\",\"deviceId\":\"";
String logHeader = "{\"info\":\"";
String commandNameHeader = "{\"name\":\"";
String closeBracket = "\"}";

String measurementResponse = "";
String identifier = "";
int responseCode = 0;

int scalePlot=10;
bool isRunning = false;
bool successfulRun = false;
byte c;
byte data_rcv[4];
int _resolution = 4096;
double voltage;
double current;

String deviceId = "dev-001";

void setup() {

  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    logger("Attempting to connect to WPA SSID");    

    delay(ONE_SECOND);
  }

  logger("You're connected to the network");
}

void loop() {  

  String command = fecthCommand();

  logger("BEFORE: command: " + command);  

  if (command == "STARTED_MEASUREMENT") {

    logger("command: " + command);

    initialize();
    
    HTTPClient http;
    http.begin(apiUrl + "/pawl/v1/api/measurement/");
    http.addHeader("Content-Type", "application/json");   

    startMeasurement();

    isRunning = true;

    while ((Serial2.available() > 0) && isRunning) {

      c = Serial2.read();

      switch (c) {
        case 0xA0:
          Serial.println("s");
          while ((Serial2.peek() == -1));
          data_rcv[0] = Serial2.read();
          while ((Serial2.peek() == -1));
          data_rcv[1] = Serial2.read();
          while ((Serial2.peek() == -1));
          data_rcv[2] = Serial2.read();
          while ((Serial2.peek() == -1));
          data_rcv[3] = Serial2.read();

          voltage = ((data_rcv[0] << 6) & 0x0FC0) | data_rcv[1];
          current = ((data_rcv[2] << 6) & 0x0FC0) | data_rcv[3];

          voltage = (double)((voltage - (_resolution / 2)) * (3.3 / _resolution));
          current = (double)((current - (_resolution / 2)) * (3.3 / _resolution))+1.25;

          measurementResponse = voltageHeader + String(voltage, 13) + currentHeader + String(current, 13) + identifierHeader + identifier + deviceIdHeader + deviceId + closeBracket;
          responseCode = http.POST(measurementResponse);
          successfulRun = true;

          break;
        case 0xB1:

          endMeasurement();
          isRunning = false;          

          if (successfulRun) {
            stoppedMeasurementCommand();
            logger("ENDRUN");
          }
          
          ESP.restart();
          ESP.restart();
          ESP.restart();         
          break;
          
        default:

          break;
      }
    }   
  }  
}

String fecthCommand() {

  String command = "";

  if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;

    http.begin(apiUrl + "/pawl/v1/api/command/" + deviceId);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();

    if (httpCode == 200) {

      command = http.getString();
      DynamicJsonDocument commandJsonDocument(3072);
      DeserializationError gateErrorDeserialize = deserializeJson(commandJsonDocument, command);
      String commandName = commandJsonDocument["name"];
      String commandIdentifier = commandJsonDocument["identifier"];
      identifier = commandIdentifier;
      command = commandName;    
    }

    http.end();

    delay(1000);   
  }

  return command;
}

void initialize() {

  Serial2.write(0xA0);
  Serial2.write(0x04);
  Serial2.write(0x04);
  Serial2.write(0);
  Serial2.write(0xAB);
}

void startMeasurement() {

  Serial2.write(0xA0);
  Serial2.write(0x01);
  Serial2.write(0xAB);
}

void endMeasurement() {

  Serial2.write(0xA0);
  Serial2.write(0x02);
  Serial2.write(0xAB);
  Serial2.write(0xC0);
}

void logger(String log) {

  int responseCode = 0;
  String logData = "";  

  HTTPClient http;
  http.begin(apiUrl + "/pawl/v1/api/pawl-logger/");
  http.addHeader("Content-Type", "application/json");

  logData = logHeader + log + closeBracket;

  responseCode = http.POST(logData);
}

void stoppedMeasurementCommand() {

  int responseCode = 0;
  String commandRequest = "";

  HTTPClient http;
  http.begin(apiUrl + "/pawl/v1/api/command/" + deviceId);
  http.addHeader("Content-Type", "application/json");

  commandRequest = commandNameHeader + "STOPPED_MEASUREMENT" + identifierHeader + deviceId + closeBracket;

  responseCode = http.PUT(commandRequest);
}
