#pragma once

#include "logger.hpp"

namespace ENGINE_NAMESPACE {

class Timer {
  public:
    Timer() {}
    ~Timer() {}

    void start();
    void stop();
    void reset();
    void time(const String& message);

    // TODO: most likely temporary for now
    static Timer global_timer;

  private:
    bool    _running         = false;
    float64 _last_checkpoint = 0.0;
};

} // namespace ENGINE_NAMESPACE