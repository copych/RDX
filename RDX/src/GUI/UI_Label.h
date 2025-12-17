#pragma once
#include "UI_Widget.h"
#include "UI_Display.h"

class UI_Label : public UI_Widget {
public:
    String text_;
    bool focusable_ = false;   // labels default: not focusable

    UI_Label(int x, int y, int w, int h, const String& txt)
        : UI_Widget(x, y, w, h), text_(txt) {}

    inline bool isFocusable() const override { return focusable_; }

    inline void setText(const String& t) {
        if (t != text_) {
            text_ = t;
            dirty_ = true;
        }
    }

    // ---------------- DRAW ----------------
    inline void draw(UI_Display& d) override {
        if (!dirty_ && !globalDirty_) return;

        // draw focus rectangle only if focusable AND focused
        if (focusable_ && focus_) {
            d.drawRect(x_, y_, w_, h_);
        }

        // text position: slightly inset, vertically centered
        int fh = d.getFontHeight() * (int)d.getTextScale();   // scaled height
        int ty = y_ + (h_ - fh) / 2;
        int tx = x_ + 1;

        d.drawText(tx, ty, text_.c_str());

        dirty_ = false;
    }

    inline void onEvent(const UI_Event&) override {
        // label does nothing
    }
};
