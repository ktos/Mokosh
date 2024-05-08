#ifndef MOKOSHSERVICE_H
#define MOKOSHSERVICE_H

#include <Arduino.h>
#include <memory>

class Mokosh;

class MokoshService
{
public:
    MokoshService()
    {
    }

    virtual bool setup(std::shared_ptr<Mokosh> mokosh) = 0;
    virtual bool isSetup()
    {
        return this->setupReady;
    }

protected:
    bool setupReady = false;
};

#endif