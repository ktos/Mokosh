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

class MokoshLogger : public MokoshService
{
public:
    // logs the message to the logger
    // the message consists of the level, function it was called in, filename with line number, time
    // and the actual message
    virtual void log(LogLevel level, const char *func, const char *file, int line, long time, const char *msg) = 0;

    // logs that the long operation is in progress
    virtual void ticker_step() = 0;

    // logs that the long operation has finished
    virtual void ticker_finish(bool success) = 0;

    // sets the log level beneath which logs are ignored
    virtual void setLevel(LogLevel level)
    {
        this->currentLevel = level;
    }

    // gets the current log level
    virtual LogLevel getLevel()
    {
        return this->currentLevel;
    }

protected:
    LogLevel currentLevel;
};

class SerialLogger : public MokoshLogger
{
public:
    virtual bool setup() override
    {
        this->setupFinished = true;
        return true;
    }

    virtual void loop() override {}

    virtual void log(LogLevel level, const char *func, const char *file, int line, long time, const char *msg) override
    {
        if (level < this->currentLevel)
            return;

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

    virtual void ticker_step() override
    {
        Serial.print(".");
    }

    virtual void ticker_finish(bool success) override
    {
        if (success)
        {
            Serial.println(" ok");
        }
        else
        {
            Serial.println(" fail");
        }
    }
};

#endif