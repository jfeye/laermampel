#include <Arduino.h>

#include "FastLED.h"

#define LEDS_PIN 4
#define NUM_LEDS 48
#define MAX_CURRENT 500
#define FRAMES_PER_SECOND 30
CRGB leds[NUM_LEDS];

#define TIMEOUT 1000

#define MAX_MIC_SENSE_FACTOR 5

#define VAL_BUF_MAX_SIZE 100
float valBufMax[VAL_BUF_MAX_SIZE];
uint8_t valBufMax_i = 0;

#define VAL_BUF_AVG_SIZE 5
float valBufAvg[VAL_BUF_AVG_SIZE];
uint8_t valBufAvg_i = 0;

float* dbLUT;
float avg = 0.0;
float envelope = 0.0;
float scale = 1.0;
uint16_t val = 0;

uint16_t baseline = 498;
uint8_t green_lim = 0;
uint8_t red_lim = 255;

uint8_t brightness = 32;
uint8_t sensitivity = 128;

uint32_t nextFrame = 0;

uint8_t inBuff[3];


// Functions
void handleSerial();
void setLEDs(uint8_t value);
uint8_t receiveBytes(uint8_t bytes);
void genLUT();


void setup(void){
	// Set up the debug connection
	Serial1.begin(9600);

	genLUT();

	// LED setup
	FastLED.addLeds<WS2812B, LEDS_PIN, GRB>(leds, NUM_LEDS);
	FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT);
}


/*
	Returns a float[] of size max(baseline, 1023-baseline)+1
	in range [0.0, 255.0]	with logarithmic scale.
	255.0 represents 0dB
	The dB value for 0.0 depends on max(baseline, 1023-baseline)
	10 bit ADC => 20*log10(1/2^10) = -60.206 dB at most
*/
void genLUT() {
	uint16_t max = max(baseline, 1023-baseline);
  if (dbLUT != 0) {
    delete [] dbLUT;
  }
  dbLUT = new float [max+1];

	for (float i=1; i<=max; i++) {
		dbLUT[i] = 20.0 * log10(i/max);
	}
	// to avoid NaN at i=0 (log10(0)=-inf) set minimum volume to
	// half of dbLUT[1], e.g. ~6dB
	float min = dbLUT[1] - 20.0 * log10(2);
	dbLUT[0] = min;

	// scale from dB to [0.0, 255.0]
	for (uint16_t i=0; i<=max; i++) {
		dbLUT[i] = 255.0 * (min - dbLUT[i]) / min;
	}
}


void loop(void) {
	// read mic signal in [0, 1023] centered around baseline
	val = analogRead(A0);

	// get absolute value of mic signal in [0, max(baseline, 1023-baseline)]
	val = val > baseline ? val - baseline : baseline - val;

	// scale mic signal according to sensitivity slider
	if (sensitivity >= 128) {
    scale = (sensitivity - 128)*(MAX_MIC_SENSE_FACTOR - 1)/128.0 + 1.0;
    val = (uint16_t)(val * scale);
  } else {
    scale = (128 - sensitivity)*(MAX_MIC_SENSE_FACTOR - 1)/128.0 + 1.0;
    val = (uint16_t)(val / scale);
  }

	// clip mic signal to [0, max(baseline, 1023-baseline)]
	val = min(max(0, val), max(baseline, 1023-baseline));

	if (millis() < nextFrame) {
		valBufMax[valBufMax_i] = dbLUT[val];
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

		// envelope follower with zero attack time
		// and linear release in range [0.0, 255.0]
		if (avg >= envelope) {
			envelope = avg;
		} else {
			envelope -= 0.1 + (envelope - avg) / 64.0;
		}
		envelope = min(max(0.0, envelope), 255.0);

		setLEDs(envelope);
		Serial.println(envelope);

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
	while (Serial1.available() < bytes && millis()-ts < TIMEOUT){
		delay(1);
	}
	if (millis()-ts >= TIMEOUT) {
		Serial.println("Serial timeout");
	}
	uint8_t n = Serial1.readBytes(inBuff, bytes);
	if (n != bytes) {
		return 0;
	}
	return 1;
}
