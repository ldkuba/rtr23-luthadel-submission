#pragma once

#include "platform/platform.hpp"

class RendererBackend {
private:
    uint64 _frame_number = 0;

public:
    RendererBackend(Platform::Surface* surface) {}
    ~RendererBackend() {}

    void increment_frame_number() { _frame_number++; }
    virtual void resized(const uint32 width, const uint32 height) {}
    virtual bool begin_frame(const float32 delta_time) { return false; }
    virtual bool end_frame(const float32 delta_time) { return false; }
};
