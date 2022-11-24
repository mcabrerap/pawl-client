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

String voltageHeader = "{\"voltage\":\"";
String currentHeader = "\",\"current\":\"";
String closeBracket = "\"}";

String logHeader = "{\"info\":\"";
String logFooter = "\"}";

String jsonResponse = "";
int responseCode = 0;

int scalePlot=10;
bool isRunning = false;
byte c;
byte data_rcv[4];
int _resolution = 4096;
double voltage;
double current;

byte myStart[3]={0xA0,0x01,0xAB};
byte myStop[3]={0xA0,0x02,0xAB};
byte mySendAll[1]={0xC0};

void setup() {

  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
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

  if (command == "START") {

    logger("command: " + command);

    // initialize();
    Serial2.write(mySendAll, 1);
    delay(1000);
    Serial2.write(myStart, 3);
    
    HTTPClient http;
    http.begin("http://192.168.0.7:3000/pawl/v1/api/response/");
    http.addHeader("Content-Type", "application/json");   

    startMeasurement();
    isRunning = true;
    // while (Serial2.available()) {
        
    //   Serial2.read();
    // }

    while ((Serial2.available() > 0) && isRunning) {
      // logger("isRunning: " + String(isRunning));

      c = Serial2.read();

      // logger("Serial2.read(): " + String(c));

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
          
          Serial.print("v:");
          Serial.print(voltage*scalePlot);
          Serial.print(",c:");
          Serial.println(current*scalePlot);
          jsonResponse = voltageHeader + String(voltage*scalePlot, 2) + currentHeader + String(current*scalePlot, 2) + closeBracket;
          responseCode = http.POST(jsonResponse);
          logger("API");
          break;
        case 0xB0:
          // ("ACK");
          logger("ACK");
          break;
        case 0xB1:
          // ("ENDRUN");
          // Serial2.write(0xA0);
          // Serial2.write(0x02);
          // Serial2.write(0xAB);
          // Serial2.write(0xC0);
          Serial2.write(myStop, 3);
          delay(1000);
          Serial2.write(mySendAll, 1);
          isRunning = false;
          logger("ENDRUN");          
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

    http.begin("http://192.168.0.7:3000/pawl/v1/api/command/");
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();

    if (httpCode == 200) {

      command = http.getString();            
    }

    http.end();

    // delay(1000);   
  }

  return command;
}

void initialize() {

  Serial2.write(0xC0);
}

void startMeasurement() {

  Serial2.write(0xA0);
  Serial2.write(0x01);
  Serial2.write(0xAB);
}

void logger(String log) {

  int responseCode = 0;
  String logData = "";

  HTTPClient http;
  http.begin("http://192.168.0.7:3000/pawl/v1/api/pawl-logger/");
  http.addHeader("Content-Type", "application/json");

  logData = logHeader + log + logFooter;

  responseCode = http.POST(logData);
}
