#include <Mokosh.hpp>
#include <MokoshMDNSService.hpp>

Mokosh m;

void setup()
{
    m.setLogLevel(LogLevel::VERBOSE);
    m.registerService(MokoshServices::MDNSService::KEY, std::make_shared<MokoshServices::MDNSService>());

    m.begin();

    auto mdns = m.getRegisteredService<MokoshServices::MDNSService>(MokoshServices::MDNSService::KEY);
    mdns->addMDNSService("TEST", "HTTP", 80);
}

void loop()
{
    m.loop();
}
