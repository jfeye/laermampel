#include <stdlib.h>
#include <Arduino.h>
#include "FastLED.h"

#define LEDS_PIN 10
#define NUM_LEDS 48
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 30
CRGB leds[NUM_LEDS];

#define TIMEOUT 1000

#define VAL_BUF_MAX_SIZE 100
uint16_t valBufMax[VAL_BUF_MAX_SIZE];
uint8_t valBufMax_i = 0;

#define VAL_BUF_AVG_SIZE 5
uint16_t valBufAvg[VAL_BUF_AVG_SIZE];
uint8_t valBufAvg_i = 0;

float avg = 0.0;
float envelope = 0.0;
float scale = 1.0;
uint16_t val = 0;
uint16_t maxVal = 0;

#define MIC_PIN A0
#define MIC_MAX_VALUE 1023 //675
uint16_t baseline = 498;
uint8_t slope = 128;
#define MAX_SLOPE 10
uint8_t green_lim = 88;
uint8_t red_lim = 255;

uint8_t brightness = 32;
#define MAX_MIC_SENSE_FACTOR 5
uint8_t sensitivity = 128;

uint32_t nextFrame = 0;

uint8_t inBuff[7];


// Functions
void handleSerial();
void setLEDs(uint8_t value);
uint8_t receiveBytes(uint8_t bytes);
uint8_t slopeVal(uint16_t val);


void setup(void){
  // Set up the debug connection
  Serial.begin(9600);

  maxVal = max(baseline, MIC_MAX_VALUE-baseline);

  // LED setup
  FastLED.addLeds<WS2812B, LEDS_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT);
}


uint8_t slopeVal(uint16_t x, uint16_t maxX) {
  float fval = (float)x/(float)maxX;
  if (slope >= 128) {
    scale = (slope - 128)*(MAX_SLOPE - 1)/127.0 + 1.0;
    return (uint8_t)(pow(fval, scale)*255.0);
  } else {
    scale = (127 - slope)*(MAX_SLOPE - 1)/127.0 + 1.0;
    return (uint8_t)(pow(fval, 1.0/scale)*255.0);
  }
}


void loop(void) {
  // read mic signal in [0, MIC_MAX_VALUE] centered around baseline
  val = analogRead(MIC_PIN);
  //Serial.println(val);

  // get absolute value of mic signal in [0, maxVal]
  val = val > baseline ? val - baseline : baseline - val;

  // scale mic signal according to sensitivity slider
  if (sensitivity >= 128) {
    scale = (sensitivity - 128)*(MAX_MIC_SENSE_FACTOR - 1)/128.0 + 1.0;
    val = (uint16_t)(val * scale);
  } else {
    scale = (128 - sensitivity)*(MAX_MIC_SENSE_FACTOR - 1)/128.0 + 1.0;
    val = (uint16_t)(val / scale);
  }

  // clip mic signal to [0, maxVal]
  val = (uint16_t)(min(max(0, val), maxVal));// * (5.0/3.3));

  if (millis() < nextFrame) {
    valBufMax[valBufMax_i] = val;
    valBufMax_i = (valBufMax_i + 1) % VAL_BUF_MAX_SIZE;
    if (valBufMax_i == 0) {
      valBufAvg[valBufAvg_i] = 0.0;
      for (uint8_t i=0; i<VAL_BUF_MAX_SIZE; i++) {
        if (valBufMax[i] > valBufAvg[valBufAvg_i]) {
          valBufAvg[valBufAvg_i] = valBufMax[i];
        }
      }
      valBufAvg_i = (valBufAvg_i + 1) % VAL_BUF_AVG_SIZE;
    }
  } else {
    // read gui parameter
    handleSerial();

    // get average volume
    avg = 0.0;
    for (uint8_t i=0; i<valBufAvg_i; i++) {
      avg += valBufAvg[i];
    }
    avg /= valBufAvg_i;

    // scale avg to [green_lim, red_lim]
    // This maps dB value to color
    if (red_lim < green_lim) {
      uint8_t tmp = red_lim;
      red_lim = green_lim;
      green_lim = tmp;
    }
    if (avg < green_lim) {
      avg = 0.0;
    } else if (avg > red_lim) {
      avg = 255.0;
    } else {
      avg = 255.0 * (float)(avg - green_lim) / (float)(red_lim - green_lim);
    }
    avg = slopeVal((uint16_t)round(avg), 255);

    // envelope follower with zero attack time
    // and linear release in range [0.0, 255.0]
    if (avg >= envelope) {
      envelope += (avg - envelope) / 32.0;
    } else {
      envelope -= (256.0 - envelope) / 64.0 ;
    }
    envelope = min(max(0.0, envelope), 255.0);

    Serial.print(avg);
    Serial.print(",");
    Serial.print(val);
    Serial.print(",");
    Serial.println(envelope);

    setLEDs(envelope);

    valBufMax_i = 0;
    valBufAvg_i = 0;
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
  while (Serial.available()) {
    //detect message start
    if (Serial.read() == 0xFF) {
      if (receiveBytes(6)) {
        brightness = inBuff[0];
        sensitivity = inBuff[1];
        baseline = (uint16_t)((inBuff[2] + 384));// * (3.3/5.0));
        slope = inBuff[3];
        green_lim = inBuff[4];
        red_lim = inBuff[5];
      }
    }
  }
}


uint8_t receiveBytes(uint8_t bytes) {
  uint32_t ts = millis();
  while (Serial.available() < bytes && millis()-ts < TIMEOUT){
    delay(1);
  }
  if (millis()-ts >= TIMEOUT) {
    Serial.println(F("Serial timeout"));
  }
  uint8_t n = Serial.readBytes(inBuff, bytes);
  if (n != bytes) {
    return 0;
  }
  return 1;
}
