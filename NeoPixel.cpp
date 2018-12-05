#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "NeoPixel.h"
#include <FastLED.h>

CRGB leds[16];
uint8_t pixnum;

void NeoPixel_setup(uint8_t pnum)
{
	pixnum = pnum;
	FastLED.addLeds<NEOPIXEL, 3>(leds, pixnum);	
}

uint32_t NeoPixel_convertColorToCode(int r, int g, int b)
{
	return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void NeoPixel_progress(uint8_t level, uint32_t color)
{
	level = level % pixnum;

	for (uint8_t i = 0; i < level; i++)
		leds[i].setColorCode(color);

	FastLED.show();
}

void NeoPixel_color(uint32_t color)
{
	FastLED.showColor(CRGB(color));
}

void NeoPixel_error(uint16_t errorCode)
{
	leds[0] = CRGB::DarkBlue;
	leds[1] = CRGB::DarkBlue;
	leds[2] = CRGB::Black;

	if (errorCode >= 10)
		leds[3] = CRGB::DarkGreen;

	if (errorCode >= 50)
		leds[3] = CRGB::DarkBlue;

	if (errorCode >= 100)
		leds[3] = CRGB::DarkRed;

	if (errorCode >= 200)
		leds[4] = CRGB::DarkRed;

	if (errorCode >= 500)
		leds[5] = CRGB::DarkRed;

	if (errorCode >= 1000)
		leds[6] = CRGB::DarkRed;

	if (errorCode >= 2000)
		leds[7] = CRGB::DarkRed;

	if (errorCode >= 5000)
		leds[3] = CRGB::Yellow;

	if (errorCode >= 10000)
		leds[4] = CRGB::Yellow;


	FastLED.show();
}

/*
* Put a value 0 to 255 in to get a color value.
* The colours are a transition r -> g -> b -> back to r
* Inspired by the Adafruit examples.
*/
uint32_t color_wheel(uint8_t pos) {
	pos = 255 - pos;
	if (pos < 85) {
		return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
	}
	else if (pos < 170) {
		pos -= 85;
		return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
	}
	else {
		pos -= 170;
		return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
	}
}

void NeoPixel_anim(Anim type, uint32_t color)
{
	switch (type)
	{
	case KnightRider1:
		for (uint8_t i = 0; i < pixnum; i++) {
			leds[i].setColorCode(color);
			FastLED.show();

			delay(50);
			leds[i] = CRGB::Black;
			FastLED.show();
		}
		leds[pixnum - 1].setColorCode(color);
		FastLED.show();
		break;

	case KnightRider2:
		for (uint8_t i = pixnum - 1; i > 0; i--) {
			leds[i].setColorCode(color);
			FastLED.show();

			delay(50);
			leds[i] = CRGB::Black;
			FastLED.show();
		}
		leds[0].setColorCode(color);
		FastLED.show();
		break;


	case RainbowCycle:
		int j = 0;

		while (j < 255) {
			for (uint16_t i = 0; i < pixnum; i++) {
				uint32_t color = color_wheel(((i * 256 / pixnum) + j) & 0xFF);
				leds[i].setColorCode(color);
			}

			j = (j + 1) & 0xFF;
			FastLED.show();
			delay(1);
		}

		break;
	}
}

void NeoPixel_animtime(Anim type, uint32_t color, uint16_t time)
{
	unsigned long start = millis();

	while (true) {
		NeoPixel_anim(type, color);
		
		if (millis() - start > time) 
			break;
	}
}