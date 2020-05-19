#pragma once

/*
	Four constants for firmware configuration:		
		UPDATE_TIME defines time between updates for sensors
		HEARTBEAT_TIME defines time between publishing next heartbeat message
		DEBUG_LEVEL defines the debug level shown on the serial port
			1 is DEBUG, 2 is INFO, 4 is WARNING, 8 is ERROR
			4 or 8 enables "RELEASE" builds
		DEV_PREFIX is a prefix used for device type recognition: 
			CHS for scales,
			CHB for barcode readers,
*/
#define UPDATE_TIME 1000
#define HEARTBEAT_TIME 10000
#define DEBUG_LEVEL 1
#define DEV_PREFIX "MOKOSH"

/*
	Virtual random sensor configuration:
		ENABLE_RAND enables usage of random "sensor"		
*/

#define ENABLE_RAND

/*
	If ENABLE_NEOPIXEL you may turn on NeoPixel support for status lights

	PIXPIN is a PIN number to which NeoPixel strip is connected
	PIXNUM is a number of pixels in the strip
*/

//#define ENABLE_NEOPIXEL
//#define PIXPIN 3
//#define PIXNUM 8