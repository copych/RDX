class UI_Page {
public:
    virtual ~UI_Page() {}

    virtual void onEnter() {}
    virtual void onExit() {}

    virtual void draw(UI_Display& d) {
        for (auto* w : widgets_)
            w->draw(d);
    }

    virtual void onInput(uint8_t encoderId, int16_t value) {
        if (encoderId < bindings_.size() && bindings_[encoderId])
            bindings_[encoderId]->onInput(encoderId, value);
    }

    inline void bindEncoder(uint8_t encId, UI_Widget* w) {
        if (encId >= bindings_.size()) bindings_.resize(encId+1, nullptr);
        bindings_[encId] = w;
    }

protected:
    std::vector<UI_Widget*> widgets_;
    std::vector<UI_Widget*> bindings_;   // encoder > widget mapping
};
