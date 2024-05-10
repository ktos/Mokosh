#ifndef DEBUGADAPTER_H
#define DEBUGADAPTER_H

#include "MokoshService.hpp"

// Debug level - starts from 0 to 6, higher is more severe
typedef enum LogLevel
{
    PROFILER = 0,
    DEBUG = 1,
    VERBOSE = 2,
    INFO = 3,
    WARNING = 4,
    ERROR = 5,
    ANY = 6
} LogLevel;

class DebugAdapter : public MokoshService
{
public:
    virtual bool isActive(LogLevel level) = 0;
    virtual void setActive(LogLevel level) = 0;
    virtual void debugf(LogLevel level, const char *func, const char *file, int line, long time, const char *msg) = 0;
};

class SerialDebugAdapter : public DebugAdapter
{
public:
    virtual bool setup(std::shared_ptr<Mokosh> mokosh)
    {
        return true;
    }

    virtual void loop() {}

    virtual bool isActive(LogLevel level)
    {
        return true;
    }

    virtual void setActive(LogLevel level) {}

    virtual void debugf(LogLevel level, const char *func, const char *file, int line, long time, const char *msg)
    {
        char lvl;
        switch (level)
        {
        case LogLevel::DEBUG:
            lvl = 'D';
            break;
        case LogLevel::ERROR:
            lvl = 'E';
            break;
        case LogLevel::INFO:
            lvl = 'I';
            break;
        case LogLevel::WARNING:
            lvl = 'W';
            break;
        case LogLevel::VERBOSE:
            lvl = 'V';
            break;
        default:
            lvl = 'A';
        }

        Serial.printf("(%c t:%ldms) (%s %s:%d) %s\n", lvl, time, func, file, line, msg);
    }
};

#endif