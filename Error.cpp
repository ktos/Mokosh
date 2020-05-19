#include "Error.h"
#include "NeoPixel.h"
#include "Debug.h"

bool errorUseBuiltinLed = true;
bool errorUseNeoPixel = false;

void Error_configure(bool useBuiltInLed, bool useNeoPixel)
{
	errorUseBuiltinLed = useBuiltInLed;
	errorUseNeoPixel = useNeoPixel;

	if (errorUseBuiltinLed) {
		pinMode(LED_BUILTIN, OUTPUT);
		digitalWrite(LED_BUILTIN, HIGH);
	}
}

void Error_showBlinking(uint16_t errorCode)
{
	if (errorUseNeoPixel) {
		Debug_print(DLVL_ERROR, "ERROR", errorCode);

#ifdef ENABLE_NEOPIXEL
		NeoPixel_error(errorCode);
#endif
		delay(60000);
	}
	else {
		for (char j = 0; j < 30; j++)
		{
			Debug_print(DLVL_ERROR, "ERROR", errorCode);

			// we cannot use LED_BUILTIN e.g. with deep sleep, as they are
			// connected to the same port
			// instead, send serial data, which causes blue led to blink
			// and maybe user will notice the pattern
			for (char i = 0; i < 10; i++)
			{
				Serial.println(errorCode);
				delay(errorCode);
			}
			delay(10000);
		}
	}

	ESP.restart();
}

void Error_show(uint16_t errorCode)
{
	if (!errorUseBuiltinLed)
		Error_showBlinking(errorCode);

	while (true)
	{
		Debug_print(DLVL_ERROR, "ERROR", errorCode);

		// blinking 10 times the error code
		for (char i = 0; i < 10; i++)
		{
			digitalWrite(LED_BUILTIN, LOW);
			delay(errorCode);
			digitalWrite(LED_BUILTIN, HIGH);
			delay(errorCode);
		}

		// and wait for 10 seconds
		delay(10 * 1000);
	}
}