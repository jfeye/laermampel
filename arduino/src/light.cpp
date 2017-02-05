#include <Arduino.h>

#include "FastLED.h"

#define LEDS_PIN 4
#define NUM_LEDS 5
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 30
#define T1_BUFF_SIZE 200
#define T2_BUFF_SIZE 5

CRGB leds[NUM_LEDS];
uint8_t brightness = 255;
uint8_t sensitivity = 128;

uint32_t nextFrame = 0;

uint16_t t1Buff[T1_BUFF_SIZE];
uint16_t t1Buff_i = 0;
uint16_t t2Buff[T2_BUFF_SIZE];
uint16_t t2Buff_i = 0;

double envbums = 0;

// Functions
void handleSerial();
void setLEDs(uint8_t value);

void setup(void){

  pinMode(D1, OUTPUT);
  // Set up the debug connection
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP(SSID,PASSWD);
  // Set up HTTP-Server
  server.on("/",serveIndex);
  server.on("/set",handleSet);
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

  // LED setup
  FastLED.addLeds<WS2812B, LEDS_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_CURRENT);
}

void loop(void){
  delayMicroseconds(100);

  uint16_t val = analogRead(A0);
  if(val > 498) t1Buff[t1Buff_i] = val-498;
  else t1Buff[t1Buff_i] = 498 - val;
  t1Buff_i = (t1Buff_i+1);

  if(t1Buff_i >= T1_BUFF_SIZE){
    t1Buff_i = 0;
    uint16_t max = 0;
    for (uint16_t i = 0; i < T1_BUFF_SIZE; i++) {
      if(t1Buff[i]> max)
        max = t1Buff[i];
    }
    t2Buff[t2Buff_i] = max;
    t2Buff_i = (t2Buff_i+1) % T2_BUFF_SIZE;
  }

  if(millis()>nextFrame){
    server.handleClient();

    uint16_t average = 0;
    for (uint16_t i = 0; i < T2_BUFF_SIZE; i++){
        average += t2Buff[i];
    }
    average /= T2_BUFF_SIZE;

    if (average > envbums) envbums = average;
    else if (average != envbums) envbums -= 0.5 + (envbums-average)/20.0;

    if(envbums>255) envbums = 255;
    setLEDs(envbums);
    nextFrame = millis() + (1000/FRAMES_PER_SECOND);
  }
}

void setLEDs(uint8_t value){
  for(int i=0; i<NUM_LEDS; i++){
    leds[i] = CHSV(96-(value*96)/255,255,255);
  }
  FastLED.setBrightness(brightness);
  FastLED.show();
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
  uint8_t res = 0;

  for(int i=0; i<server.args();i++){

    if(server.argName(i) == "brightness") {
      brightness = atoi(server.arg(i).c_str());
    }else if(server.argName(i) == "sensitivity") {
      sensitivity = atoi(server.arg(i).c_str());
    }else{
        res = 1; // error arg not found
    }
    if (res != 0) {
      server.send(200, "text/html", "ERROR on " + server.argName(i) + " : " + server.arg(i));
    }
  }
  if(res == 0 && server.args()>0) server.send(200, "text/html", "OK");
  else server.send(404, "text/html", "ERROR");
}
