#ifndef VKTEST_TIMER_H
#define VKTEST_TIMER_H

#include <SDL2/SDL.h>

class Timer {
public:
    Timer(void);
    virtual ~Timer(void) { };
    double Elapsed(void);
private:
    uint64_t m_start;
    uint64_t m_freq;
};

#endif
