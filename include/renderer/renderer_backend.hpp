#pragma once

#include "platform/platform.hpp"

class RendererBackend {
private:
    uint64 _frame_number = 0;

public:
    RendererBackend(Platform::Surface* surface) {}
    ~RendererBackend() {}

    void increment_frame_number() { _frame_number++; }
    virtual void resized(uint32 width, uint32 height) {}
    virtual bool begin_frame(float32 delta_time) { return false; }
    virtual bool end_frame(float32 delta_time) { return false; }
    virtual void wait_for_shutdown() {}
};
