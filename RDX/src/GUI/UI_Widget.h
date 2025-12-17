// UI_Widget.h
#pragma once
#include <Arduino.h>
#include "UI_Display.h"
#include "RDX_State.h"

class UI_Widget {
public:
    virtual ~UI_Widget() {}

    inline void setPos(int x, int y) { x_ = x; y_ = y; }
    inline void setSize(int w, int h) { w_ = w; h_ = h; }

    virtual void draw(UI_Display& d) = 0;

    // Event coming from global input dispatcher
    virtual void onInput(uint8_t inputId, int16_t value) {}

protected:
    int x_ = 0, y_ = 0, w_ = 0, h_ = 0;
};
