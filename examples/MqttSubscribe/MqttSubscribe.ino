#include <Mokosh.hpp>

Mokosh m;

void setup()
{
    m.setLogLevel(LogLevel::VERBOSE);
    m.begin();

    auto mqtt = m.getMqttService();

    mqtt->onMessage = [](String topic, uint8_t *msg, unsigned int length)
    {
#if defined(ESP32)
        auto mess = String(msg, length);
#elif defined(ESP8266)
        char ch[length];
        strcpy(ch, (const char *)msg);
        String mess(ch);
#endif

        mdebugI("Received message on topic %s: %s", topic.c_str(), mess.c_str());
    };
    mqtt->subscribe("some/topic");
}

void loop()
{
    m.loop();
}
