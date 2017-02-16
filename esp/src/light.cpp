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

uint8_t brightness = 0;
uint8_t sensitivity = 0;

void sendSerial() {
  Serial.write(0xff);
  Serial.write(brightness);
  Serial.write(sensitivity);
}

void setup(void){
  // Set up the debug connection
  Serial.begin(9600);
  delay(250);

  // read status from EEPROM
  EEPROM.begin(512);
  delay(250);
  brightness = EEPROM.read(0);
  sensitivity = EEPROM.read(1);
  
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
       } else {
         server.send(404, "text/plain", "Invalid value: " + server.arg(i));
       }
     } else {
       server.send(404, "text/plain", "Invalid argument: " + server.argName(i));
     }
   }
}

void handleSet() {
  uint8_t res = 0;
  uint8_t old_brightness = brightness;
  uint8_t old_sensitivity = sensitivity;

  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "b") {
      brightness = atoi(server.arg(i).c_str());
    } else if (server.argName(i) == "s") {
      sensitivity = atoi(server.arg(i).c_str());
    } else {
        res = 1; // error arg not found
    }
    if (res != 0) {
      server.send(404, "text/html", "ERROR on " + server.argName(i) + " : " + server.arg(i));
    }
  }

  if (res == 0 && server.args() > 0) {
    sendSerial();
    if (old_brightness != brightness && brightness > 0) {
      EEPROM.write(0, brightness);
    }
    if (old_sensitivity != sensitivity && sensitivity > 0) {
      EEPROM.write(1, sensitivity);
    }
    EEPROM.commit();
    server.send(200, "text/html", "OK");
  } else {
    server.send(404, "text/html", "ERROR");
  }
}
