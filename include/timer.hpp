#pragma once

#include <chrono>

class Timer {
public:
    Timer(std::chrono::steady_clock::duration interval, void (*callback)(std::chrono::steady_clock::duration&)) 
        : m_interval(interval), m_current(0), m_callback(callback) 
    {}

    void update(std::chrono::steady_clock::duration& frame_time) {
        m_current += frame_time;
        while(m_current > m_interval) {
            m_current = m_current.zero();
            m_callback(frame_time);
        }
    }

private:
    std::chrono::steady_clock::duration m_interval, m_current;
    void (*m_callback)(std::chrono::steady_clock::duration&);
};

