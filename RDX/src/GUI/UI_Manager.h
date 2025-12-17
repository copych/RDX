#pragma once
#include "UI_Page.h"
#include "UI_Display.h"
#include "UI_InputDefs.h"   // event structs

class UI_Manager {
public:
    inline void setDisplay(UI_Display* d) { disp_ = d; }

    // assign pages to IDs corresponding to button shortcuts
    inline void bindPage(uint8_t pageId, UI_Page* page) {
        if (pageId < MAX_PAGES) pages_[pageId] = page;
    }

    inline void setPage(uint8_t pageId) {
        if (pageId >= MAX_PAGES) return;
        UI_Page* next = pages_[pageId];
        if (!next) return;

        if (curPage_) curPage_->onExit();

        curPage_ = next;
        curPageId_ = pageId;

        updateLEDs();

        curPage_->onEnter();
    }

    // main event dispatcher --
    // routes hardware controls appropriately
    inline void handleInput(const UIInputEvent& ev) {
        switch (ev.type) {

            case UIInputType::BUTTON:
                handleButton(ev.id, ev.value);
                break;

            case UIInputType::ENCODER:
            case UIInputType::ENCODER_CLICK:
                if (curPage_) curPage_->onInput(ev.id, ev.value);
                break;

            default:
                break;
        }
    }

    inline void draw() {
        if (!disp_ || !curPage_) return;
        disp_->clear();
        curPage_->draw(*disp_);
        disp_->update();
    }

private:
    static constexpr uint8_t MAX_PAGES = 20;

    UI_Display* disp_ = nullptr;
    UI_Page*    pages_[MAX_PAGES] = { nullptr };
    UI_Page*    curPage_ = nullptr;
    uint8_t     curPageId_ = 0;

    // -------------------
    // Page-select buttons
    // -------------------
    inline void handleButton(uint8_t id, int16_t value) {
        if (value == 0) return; // only process “press” events
        setPage(id);
    }

    // -------------------
    // LED logic stub
    // -------------------
    inline void updateLEDs() {
        for (int i = 0; i < MAX_PAGES; ++i)
            setLED(i, i == curPageId_);
    }

    inline void setLED(uint8_t id, bool on) {
        // user must implement
        if (ledCallback_) ledCallback_(id, on);
    }

public:
    // hook for external LED driver
    void setLEDCallback(void (*cb)(uint8_t,bool)) {
        ledCallback_ = cb;
    }

private:
    void (*ledCallback_)(uint8_t, bool) = nullptr;
};
