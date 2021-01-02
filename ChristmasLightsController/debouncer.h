#ifndef ANDREYVIT_DEBOUNCE_H
#define ANDREYVIT_DEBOUNCE_H

template <int triggerLevel, unsigned cooldownDelayMs>
class Debouncer {
public:
    Debouncer()
        : lastChangeTime(0)
        , lastReading(triggerLevel == LOW ? HIGH : LOW)
        , steadyReading(triggerLevel == LOW ? HIGH : LOW)
    {}

    bool triggered(int reading, unsigned long now) {
        if (reading != lastReading) {
            lastChangeTime = now;
            lastReading = reading;
        }
        if (reading != steadyReading && now > lastChangeTime + cooldownDelayMs) {
            steadyReading = reading;
            if (steadyReading == triggerLevel) {
                return true;
            }
        }
        return false;
    }

private:
    unsigned long lastChangeTime;
    int lastReading;
    int steadyReading;
};

#endif // ANDREYVIT_DEBOUNCE_H