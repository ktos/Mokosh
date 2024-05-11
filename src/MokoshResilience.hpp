#ifndef MOKOSHRESILIENCE_H
#define MOKOSHRESILIENCE_H

namespace MokoshResilience
{
    // class for counting failures and finally returning a general failure
    class CounterCircuitBreaker
    {
    public:
        CounterCircuitBreaker(int limit = 3)
        {
            this->limit = limit;
        }

        void increment()
        {
            this->counter++;
            mlogV("Incremented failure counter to %d!", this->counter);

            if (this->counter >= this->limit)
                mlogE("Failure counter exceeded threshold!");
        }

        void reset()
        {
            this->counter = 0;
        }

        bool isFail()
        {
            return this->counter >= this->limit;
        }

    private:
        int limit = 0;
        int counter = 0;
    };

    // class for retrying things with delay between trials
    class Retry
    {
    public:
        // will retry the operation if returned false, each time delaying it
        // a bit longer, until the name of trials is being exceeded
        static bool retry(std::function<bool(void)> operation, int trials = 3, long delayTime = 100, int delayFactor = 2)
        {
            int i = 0;
            while (i < trials)
            {
                if (operation())
                    return true;

                long time = delayTime * power(delayFactor, i + 1);
                i++;
                if (i >= trials)
                    break;

                mlogV("Resilience operation failed, retrying in %d", time);
                delay(time);
            }

            mlogV("Resilience operation failed after %d trials, giving up!", trials);
            return false;
        }

    private:
        static long power(int base, int exponent)
        {
            if (exponent == 0)
            {
                return 1;
            }

            long result = base;
            for (int i = 1; i < exponent; i++)
            {
                result *= base;
            }
            return result;
        }
    };

}

#endif