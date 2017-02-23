#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <EEPROM.h>

#include "send_progmem.h"
#include "html_index.h"


#define HOSTNAME      "laermampel" // mDNS-adress (http://HOSTNAME.local/)
#define SSID          "Laermampel"
#define PASSWD        "12341234"

ESP8266WebServer server(80);     // handles networking and provides http requests

// Functions
void serveIndex();
void handleSet();
void handleOther();
void handleGet();
void sendSerial();
void storeValues();
void readValues();

uint8_t brightness = 0;
uint8_t sensitivity = 0;
uint8_t baseline = 0;
uint8_t slope = 0;
uint8_t green_lim = 0;
uint8_t red_lim = 0;


void setup(void){
  // Set up the debug connection
  Serial.begin(9600);
  delay(250);

  // read status from EEPROM
  EEPROM.begin(512);
  delay(250);
  readValues();

  // Set up WiFi
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP(SSID, PASSWD);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("\nAP IP address: ");
  Serial.println(myIP);

  // Set MDNS address ("hostname.local")
  if (MDNS.begin(HOSTNAME, myIP)) {
    Serial.print("AP mDNS address: http://");
    Serial.print(HOSTNAME);
    Serial.println(".local/");
  } else {
    Serial.println("Error: mDNS fail");
  }

  // Set up HTTP-Server
  server.on("/", serveIndex);
  server.on("/set", handleSet);
  server.on("/get", handleGet);
  server.onNotFound(handleOther);
  server.begin();

  // light goes on
  sendSerial();

  Serial.println("Setup finished!\n");
}


void loop(void) {
  server.handleClient();
}


void serveIndex() {
  server.send(200, "text/html", FPSTR(html_index));
  sendProgmem(&server,html_index);
}


void handleOther() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send ( 404, "text/plain", message );
}


void handleGet() {
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "v") {
       if (server.arg(i) == "b") {
         server.send(200, "text/plain", String(brightness, DEC));
       } else if (server.arg(i) == "s") {
         server.send(200, "text/plain", String(sensitivity, DEC));
       } else if (server.arg(i) == "bl") {
         server.send(200, "text/plain", String(baseline, DEC));
       } else if (server.arg(i) == "sl") {
         server.send(200, "text/plain", String(slope, DEC));
       } else if (server.arg(i) == "gl") {
         server.send(200, "text/plain", String(green_lim, DEC));
       } else if (server.arg(i) == "rl") {
         server.send(200, "text/plain", String(red_lim, DEC));
       } else {
         server.send(404, "text/plain", "Invalid value: " + server.arg(i));
       }
     } else {
       server.send(404, "text/plain", "Invalid argument: " + server.argName(i));
     }
   }
}


void sendSerial() {
  Serial.write(0xff);
  Serial.write(brightness);
  Serial.write(sensitivity);
  Serial.write(baseline);
  Serial.write(slope);
  Serial.write(green_lim);
  Serial.write(red_lim);
}


void readValues() {
  brightness  = EEPROM.read(0);
  sensitivity = EEPROM.read(1);
  baseline    = EEPROM.read(2);
  slope       = EEPROM.read(3);
  green_lim   = EEPROM.read(4);
  red_lim     = EEPROM.read(5);
}

void storeValues() {
  EEPROM.write(0, brightness);
  EEPROM.write(1, sensitivity);
  EEPROM.write(2, baseline);
  EEPROM.write(3, slope);
  EEPROM.write(4, green_lim);
  EEPROM.write(5, red_lim);
  EEPROM.commit();
}

void handleSet() {
  uint8_t res = 0;

  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "b") {
      brightness = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "s") {
      sensitivity = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "bl") {
      baseline = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "sl") {
      slope = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "gl") {
      green_lim = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "rl") {
      red_lim = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "st") {
      storeValues();
    } else {
      res = 1; // error arg not found
    }
    if (res != 0) {
      server.send(404, "text/plain", "ERROR: Invalid argument: " + server.argName(i) + "=" + server.arg(i));
    }
  }

  if (res == 0 && server.args() > 0) {
    sendSerial();
    server.send(200, "text/plain", "Serial data sent successfully");
  } else {
    server.send(404, "text/plain", "ERROR: No arguments in request");
  }
}
