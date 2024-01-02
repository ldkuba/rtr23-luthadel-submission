#include "timer.hpp"

namespace ENGINE_NAMESPACE {

Timer Timer::global_timer {};

void Timer::start() { _running = true; }
void Timer::stop() { _running = false; }
void Timer::reset() { _last_checkpoint = Platform::get_absolute_time(); }
void Timer::time(const String& message) {
    if (!_running) return;
    float64 current  = Platform::get_absolute_time();
    float64 elapsed  = (current - _last_checkpoint) * 1000;
    _last_checkpoint = current;
    Logger::log(message, elapsed, "ms");
}

} // namespace ENGINE_NAMESPACE