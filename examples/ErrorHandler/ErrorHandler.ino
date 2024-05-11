#include <Mokosh.hpp>

Mokosh m;

void on_error(int code)
{
    if (code == 201)
    {
        mlogE("Error 201!");
        pinMode(LED_BUILTIN, HIGH);
        delay(1000);
        pinMode(LED_BUILTIN, LOW);
    }

    if (code == 202)
    {
        mlogE("Error 202!");
    }
}

void setup()
{
    pinMode(2, OUTPUT);
    // custom error handler
    m.onError = on_error;

    m.setLogLevel(LogLevel::VERBOSE);

    m.begin();
}

void loop()
{
    delay(1000);
    m.error(201); // error code 201
    delay(1000);
    m.error(202);

    m.loop();
}