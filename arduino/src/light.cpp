#include <Arduino.h>

#include "FastLED.h"

#define LEDS_PIN 4
#define NUM_LEDS 60
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 30
#define T1_BUFF_SIZE 200
#define T2_BUFF_SIZE 5

#define TIMEOUT 1000

CRGB leds[NUM_LEDS];
uint8_t brightness = 255;
uint8_t sensitivity = 128;

uint32_t nextFrame = 0;

uint16_t t1Buff[T1_BUFF_SIZE];
uint16_t t1Buff_i = 0;
uint16_t t2Buff[T2_BUFF_SIZE];
uint16_t t2Buff_i = 0;

uint8_t inBuff[3];


double envbums = 0;

// Functions
void handleSerial();
void setLEDs(uint8_t value);
uint8_t receiveBytes(uint8_t bytes);

void setup(void){
  // Set up the debug connection
  Serial1.begin(9600);

  // LED setup
  FastLED.addLeds<WS2812B, LEDS_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_CURRENT);
}

void loop(void){
  uint16_t val = analogRead(A0);
  float y = 1.0;

  t1Buff[t1Buff_i] = val;
  t1Buff_i++;

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
    handleSerial();

    uint16_t average = 0;
    for (uint16_t i = 0; i < T2_BUFF_SIZE; i++){
        average += t2Buff[i];
    }
    average /= T2_BUFF_SIZE;
    if(average>500) average -= 500;
    else average = 0;

    if (sensitivity >= 128) {
      y = (sensitivity - 128)*3.0/128.0 + 1.0;
      average = (uint16_t)(average * y);
    } else {
      y = (128 - sensitivity)*3.0/128.0 + 1.0;
      average = (uint16_t)(average / y);
    }

    if (average > envbums) envbums = average;
    else if (average != envbums) envbums -= 0.5 + (envbums-average)/16.0;

    //Serial1.println(String(average) + " " + String(envbums));

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


void handleSerial(){
  while(Serial1.available()){
    //detect message start
    if(Serial1.read() == 0xFF) {
      if (receiveBytes(2)){
        brightness = inBuff[0];
        sensitivity = inBuff[1];
      }
    }
  }
}

uint8_t receiveBytes(uint8_t bytes){
  uint32_t ts = millis();
  while(Serial1.available() < bytes && millis()-ts < TIMEOUT) delay(1);
  uint8_t n = Serial1.readBytes(inBuff, bytes);
  if(n != bytes){
    return 0;
  }
  return 1;
}
