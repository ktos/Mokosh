#pragma once

#ifndef _NEOPIXEL_h
#define _NEOPIXEL_h

#include "Arduino.h"

enum Anim { KnightRider1, KnightRider2, RainbowCycle };
enum Colors { BrightRed = 16711680, Black = 0, Green = 32768, Lime = 65280, Orange = 0xFF3000 };

// sets up the NeoPixel strip
void NeoPixel_setup(uint8_t pnum);

// sets up the NeoPixel strip in "new" mode, with rgb colors
void NeoPixel_setup(uint8_t pnum, bool grb);

// sets the strip to show progress
void NeoPixel_progress(uint8_t level, uint32_t color);

// sets LED strip to defined color
void NeoPixel_color(uint32_t color);

// sets LED strip to show the error code
void NeoPixel_error(uint16_t errorCode);

// animates the LED strip
void NeoPixel_anim(Anim type, uint32_t color);

// animates the LED strip
void NeoPixel_animtime(Anim type, uint32_t color, uint16_t time);

// converts the r, g and b values to uint32_t color code
uint32_t NeoPixel_convertColorToCode(int r, int g, int b);

#endif