#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "FastLED.h"

#include "send_progmem.h"
#include "html_index.h"


#define HOSTNAME      "derhut" // mDNS-adress (http://HOSTNAME.local/)
#define SSID          "Der Hut"
#define PASSWD        "12341234"
#define LEDS_PIN 4
#define NUM_LEDS 5
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 10

ESP8266WebServer server(80);     // handles networking and provides http requests

CRGB leds[NUM_LEDS];

// Functions
void serveIndex();
void handleSet();
void handleOn();
void handleOff();
void handleRotate();
void handleOther();

void setup(void){

  // LED setup
  FastLED.addLeds<WS2812B, LEDS_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_CURRENT);

  // Set up the debug connection
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP(SSID,PASSWD);
  // Set up HTTP-Server
  server.on("/",serveIndex);
  server.on("/set",handleSet);
  server.on("/on",handleOn);
  server.on("/off",handleOff);
  server.on("/rotate",handleRotate);
  server.onNotFound(handleOther);
  server.begin();

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Set MDNS address ("hostname.local")
  if (!MDNS.begin(HOSTNAME,myIP)) {
    Serial.println("Fehler: MDNS fail");
  }

  Serial.println("Setup finished!");
  Serial.println("");
}

void loop(void){
  server.handleClient();
  FastLED.delay(1000/FRAMES_PER_SECOND);

  // TODO update leds

}

void serveIndex(){
  server.send(200, "text/html", FPSTR(html_index));
  sendProgmem(&server,html_index);
}

void handleOther(){
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

void handleSet(){
  server.send ( 200, "text/plain", "OK" );
}

void handleOn(){
  server.send ( 200, "text/plain", "OK" );
}

void handleOff(){
  server.send ( 200, "text/plain", "OK" );
}

void handleRotate(){
  server.send ( 200, "text/plain", "OK" );
}
