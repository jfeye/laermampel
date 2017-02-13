#include <Arduino.h>

#include "FastLED.h"

#define LEDS_PIN 4
#define NUM_LEDS 48
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 30
#define T1_BUFF_SIZE 100
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

uint16_t zeroline = 498;
uint8_t start_sensing = 32;
uint8_t peak_threshold = 64;
uint16_t oldval = 0;

double envelope = 0;

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

  val = val > zeroline ? val - zeroline : zeroline - val;
  if (val < start_sensing) {
	 val = 0;
  } else if (val < 512) {
	val = (uint16_t)(((float)(val - start_sensing))/((float)(512 - start_sensing))*512);  
  } else {
	val = 512;  
  }
  oldval = val;
  //Serial.println(val);

  float y = 1.0;

  t1Buff[t1Buff_i] = val;
  t1Buff_i++;

  if(t1Buff_i >= T1_BUFF_SIZE){
    t1Buff_i = 0;
    uint16_t max = 0;
    for (uint16_t i = 0; i < T1_BUFF_SIZE; i++) {
      if (t1Buff[i] > max)
        max = t1Buff[i];
    }
    t2Buff[t2Buff_i] = max;
    t2Buff_i = (t2Buff_i+1) % T2_BUFF_SIZE;
  }

  if (millis() > nextFrame) {
    handleSerial();

    uint16_t average = 0;
    for (uint16_t i = 0; i < T2_BUFF_SIZE; i++){
        average += t2Buff[i];
    }
    average /= T2_BUFF_SIZE;

    if (average < peak_threshold)
      average = 0;
    else 
      average = (uint16_t)( 255.0 * (pow(2, ( ((float)(average - peak_threshold)) / ((float)(512-peak_threshold)) ))-1) / 1.0 );
    if (sensitivity >= 128) {
      y = (sensitivity - 128)*3.0/128.0 + 1.0;
      average = (uint16_t)(average * y);
    } else {
      y = (128 - sensitivity)*3.0/128.0 + 1.0;
      average = (uint16_t)(average / y);
    }
    //Serial.println(average);

    if (average >= envelope)
      envelope += (average-envelope)/32.0;
    else
      envelope -= 0.1 + (envelope-average)/64.0;

    if(envelope>255) envelope = 255;
    setLEDs(envelope);
    //Serial.println(String(average) + " " + String(envelope));
    Serial.println(envelope);
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
  while (Serial1.available()) {
    //detect message start
    if (Serial1.read() == 0xFF) {
      if (receiveBytes(2)) {
        brightness = inBuff[0];
        sensitivity = inBuff[1];
      }
    }
  }
}

uint8_t receiveBytes(uint8_t bytes) {
  uint32_t ts = millis();
  while (Serial1.available() < bytes && millis()-ts < TIMEOUT) delay(1);
  uint8_t n = Serial1.readBytes(inBuff, bytes);
  if (n != bytes) {
    return 0;
  }
  return 1;
}
