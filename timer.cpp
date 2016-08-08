#include "timer.h"

Timer::Timer(void)
{
    m_start = SDL_GetPerformanceCounter();
    m_freq = SDL_GetPerformanceFrequency();
}

double Timer::Elapsed(void)
{
    uint64_t diff = SDL_GetPerformanceCounter() - m_start;

    return static_cast<double>(diff) / static_cast<double>(m_freq);
}

