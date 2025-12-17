// InputManager.h
#pragma once

#include "mux4067.h"
#include "encoder.h"
#include "button.h"
#include <vector>
#include <functional>

class InputManager {
public:
    struct MuxedEncoderConfig {
        uint8_t id;
        uint8_t muxerA;
        uint8_t pinA;
        uint8_t muxerB;
        uint8_t pinB;
        MuxEncoder::encMode mode;
        gpio_num_t pcnt_gpio;
        pcnt_unit_t pcnt_unit;
    };

    struct DirectEncoderConfig {
        uint8_t id;
        uint8_t pinA;
        uint8_t pinB;
        MuxEncoder::encMode mode;
        gpio_num_t pcnt_gpio;
        pcnt_unit_t pcnt_unit;
    };

    struct MuxedButtonConfig {
        uint8_t id;
        uint8_t muxer;
        uint8_t pin;
        bool autoClick;
        bool lateClick;
        uint32_t riseTime;
        uint32_t fallTime;
        uint32_t longPressTime;
        uint32_t autoFireTime;
    };

    struct DirectButtonConfig {
        uint8_t id;
        uint8_t gpioPin;
        bool activeLow;
        bool autoClick;
        bool lateClick;
        uint32_t riseTime;
        uint32_t fallTime;
        uint32_t longPressTime;
        uint32_t autoFireTime;
    };

    InputManager(Mux4067stack* mux = nullptr) : _mux(mux) {
        if (_mux) {
            _readings = _mux->getReadingsPointer();
        }
    }

    void setMultiplexer(Mux4067stack* mux) {
        _mux = mux;
        if (_mux) {
            _readings = _mux->getReadingsPointer();
        }
    }

    // Multiplexed encoder methods
    void addMuxedEncoder(uint8_t id, uint8_t muxerA, uint8_t pinA, uint8_t muxerB, uint8_t pinB, 
                        MuxEncoder::encMode mode = MuxEncoder::MODE_FULL_STEP,
                        gpio_num_t pcnt_gpio = GPIO_NUM_NC,
                        pcnt_unit_t pcnt_unit = PCNT_UNIT_0) {
        MuxedEncoderConfig config = {id, muxerA, pinA, muxerB, pinB, mode, pcnt_gpio, pcnt_unit};
        _muxedEncoderConfigs.push_back(config);
    }

    // Direct encoder methods
    void addDirectEncoder(uint8_t id, uint8_t pinA, uint8_t pinB,
                         MuxEncoder::encMode mode = MuxEncoder::MODE_FULL_STEP,
                         gpio_num_t pcnt_gpio = GPIO_NUM_NC,
                         pcnt_unit_t pcnt_unit = PCNT_UNIT_0) {
        DirectEncoderConfig config = {id, pinA, pinB, mode, pcnt_gpio, pcnt_unit};
        _directEncoderConfigs.push_back(config);
        
        // Set up GPIO pins
        pinMode(pinA, INPUT_PULLUP);
        pinMode(pinB, INPUT_PULLUP);
    }

    // Multiplexed button methods
    void addMuxedButton(uint8_t id, uint8_t muxer, uint8_t pin,
                       bool autoClick = false, bool lateClick = false,
                       uint32_t riseTime = 20, uint32_t fallTime = 10,
                       uint32_t longPressTime = 550, uint32_t autoFireTime = 500) {
        MuxedButtonConfig config = {id, muxer, pin, autoClick, lateClick, riseTime, fallTime, longPressTime, autoFireTime};
        _muxedButtonConfigs.push_back(config);
    }

    // Direct button methods
    void addDirectButton(uint8_t id, uint8_t gpioPin, bool activeLow = true,
                        bool autoClick = false, bool lateClick = false,
                        uint32_t riseTime = 20, uint32_t fallTime = 10,
                        uint32_t longPressTime = 550, uint32_t autoFireTime = 500) {
        DirectButtonConfig config = {id, gpioPin, activeLow, autoClick, lateClick, riseTime, fallTime, longPressTime, autoFireTime};
        _directButtonConfigs.push_back(config);
        
        // Set up GPIO pin
        pinMode(gpioPin, activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
    }

    void initialize(std::function<void(int, int)> encoderCallback,
                   std::function<void(int, MuxButton::btnEvents)> buttonCallback) {
        _encoderCallback = encoderCallback;
        _buttonCallback = buttonCallback;

        // Initialize multiplexed encoders
        if (_mux) {
            for (const auto& config : _muxedEncoderConfigs) {
                MuxEncoder enc;
                enc.bind(config.id, 
                        &(_readings->Y[config.muxerA][config.pinA]),
                        &(_readings->Y[config.muxerB][config.pinB]),
                        _encoderCallback,
                        config.mode,
                        config.pcnt_gpio,
                        config.pcnt_unit);
                _muxedEncoders.push_back(enc);
            }

            // Initialize multiplexed buttons
            for (const auto& config : _muxedButtonConfigs) {
                MuxButton btn;
                btn.bind(config.id,
                        &(_readings->Y[config.muxer][config.pin]),
                        _buttonCallback);
                
                // Set button parameters
                btn.setAutoClick(config.autoClick);
                btn.enableLateClick(config.lateClick);
                btn.setRiseTimeMs(config.riseTime);
                btn.setFallTimeMs(config.fallTime);
                btn.setLongPressDelayMs(config.longPressTime);
                btn.setAutoFirePeriodMs(config.autoFireTime);
                
                _muxedButtons.push_back(btn);
            }
        }

        // Initialize direct encoders
        for (const auto& config : _directEncoderConfigs) {
            DirectEncoder enc;
            enc.id = config.id;
            enc.pinA = config.pinA;
            enc.pinB = config.pinB;
            enc.mode = config.mode;
            enc.callback = _encoderCallback;
            
            // Initialize encoder state
            enc.oldState = (digitalRead(config.pinA) << 1) | digitalRead(config.pinB);
            enc.newState = enc.oldState;
            enc.accumulator = 0;
            
            _directEncoders.push_back(enc);
        }

        // Initialize direct buttons
        for (const auto& config : _directButtonConfigs) {
            DirectButton btn;
            btn.id = config.id;
            btn.gpioPin = config.gpioPin;
            btn.activeLow = config.activeLow;
            btn.callback = _buttonCallback;
            
            // Set button parameters
            btn.setAutoClick(config.autoClick);
            btn.enableLateClick(config.lateClick);
            btn.setRiseTimeMs(config.riseTime);
            btn.setFallTimeMs(config.fallTime);
            btn.setLongPressDelayMs(config.longPressTime);
            btn.setAutoFirePeriodMs(config.autoFireTime);
            
            _directButtons.push_back(btn);
        }
    }

    void process() {
        // Read multiplexer inputs if available
        if (_mux) {
            _mux->readAll();
            
            // Process multiplexed encoders
            for (auto& encoder : _muxedEncoders) {
                encoder.process();
            }
            
            // Process multiplexed buttons
            for (auto& button : _muxedButtons) {
                button.process();
            }
        }
        
        // Process direct encoders
        for (auto& encoder : _directEncoders) {
            encoder.process();
        }
        
        // Process direct buttons
        for (auto& button : _directButtons) {
            button.process();
        }
        
        taskYIELD();
    }

    void clearAll() {
        _muxedEncoderConfigs.clear();
        _directEncoderConfigs.clear();
        _muxedButtonConfigs.clear();
        _directButtonConfigs.clear();
        _muxedEncoders.clear();
        _directEncoders.clear();
        _muxedButtons.clear();
        _directButtons.clear();
    }

    // Utility methods
    uint32_t getMuxedEncoderCount() const { return _muxedEncoders.size(); }
    uint32_t getDirectEncoderCount() const { return _directEncoders.size(); }
    uint32_t getTotalEncoderCount() const { return _muxedEncoders.size() + _directEncoders.size(); }
    uint32_t getMuxedButtonCount() const { return _muxedButtons.size(); }
    uint32_t getDirectButtonCount() const { return _directButtons.size(); }
    uint32_t getTotalButtonCount() const { return _muxedButtons.size() + _directButtons.size(); }

private:
    // Helper class for direct encoders
    class DirectEncoder {
    public:
        uint8_t id;
        uint8_t pinA;
        uint8_t pinB;
        MuxEncoder::encMode mode;
        std::function<void(int, int)> callback;
        
        int oldState;
        int newState;
        int accumulator;
        
        void process() {
            if (!callback) return;
            
            // Read current state
            unsigned int clk = (digitalRead(pinA) == LOW);
            unsigned int dt = (digitalRead(pinB) == LOW);
            
            newState = (clk | (dt << 1));
            if (newState != oldState) {
                int stateMux = newState | (oldState << 2);
                oldState = newState;
                
                int delta = 0;
                if (mode == MuxEncoder::MODE_HALF_STEP || mode == MuxEncoder::MODE_FULL_STEP) {
                    // Use the step increment tables from the original MuxEncoder
                    delta = MuxEncoder::stepIncrement[mode][stateMux];
                } else {
                    // Handle double and quad step modes
                    delta = MuxEncoder::stepIncrement[MuxEncoder::MODE_HALF_STEP][stateMux];
                    accumulator += delta;
                    
                    if (newState == 0) {
                        int needed = (mode == MuxEncoder::MODE_DOUBLE_STEP) ? 2 : 4;
                        if (abs(accumulator) >= needed) {
                            delta = (accumulator > 0) ? 1 : -1;
                            accumulator = 0;
                        } else {
                            accumulator = 0;
                            delta = 0;
                        }
                    } else {
                        delta = 0;
                    }
                }
                
                if (delta != 0) {
                    callback(id, delta);
                }
            }
        }
        
        bool isHiSpeed() const {
            // Simple speed detection - you can enhance this if needed
            // For now, return false as direct encoders don't have PCNT support
            return false;
        }
    };

    // Helper class for direct buttons
    class DirectButton {
    public:
        uint8_t id;
        uint8_t gpioPin;
        bool activeLow;
        std::function<void(int, MuxButton::btnEvents)> callback;
        
        void process() {
            if (!callback) return;
            
            bool currentState = digitalRead(gpioPin);
            bool buttonActive = activeLow ? !currentState : currentState;
            
            unsigned long currentMillis = millis();
            
            if (buttonActive != lastState) {
                if (buttonActive) {
                    // Button pressed
                    pressTime = currentMillis;
                    pressed = true;
                    callback(id, MuxButton::EVENT_TOUCH);
                } else {
                    // Button released
                    releaseTime = currentMillis;
                    callback(id, MuxButton::EVENT_RELEASE);
                    
                    // Check for click
                    if (currentMillis - pressTime < longPressThreshold) {
                        callback(id, MuxButton::EVENT_CLICK);
                    }
                    pressed = false;
                    longPressed = false;
                }
                lastState = buttonActive;
            }
            
            // Check for long press
            if (pressed && !longPressed && (currentMillis - pressTime > longPressThreshold)) {
                longPressed = true;
                callback(id, MuxButton::EVENT_LONGPRESS);
            }
            
            // Check for auto click
            if (autoClickEnabled && longPressed && (currentMillis - lastAutoClickTime > autoClickInterval)) {
                lastAutoClickTime = currentMillis;
                callback(id, MuxButton::EVENT_AUTOCLICK);
            }
        }
        
        void setAutoClick(bool enabled) { autoClickEnabled = enabled; }
        void enableLateClick(bool enabled) { lateClickEnabled = enabled; }
        void setRiseTimeMs(uint32_t ms) { riseThreshold = ms; }
        void setFallTimeMs(uint32_t ms) { fallThreshold = ms; }
        void setLongPressDelayMs(uint32_t ms) { longPressThreshold = ms; }
        void setAutoFirePeriodMs(uint32_t ms) { autoClickInterval = ms; }
        bool isPressed() const { return pressed; }
        
    private:
        bool lastState = false;
        bool pressed = false;
        bool longPressed = false;
        unsigned long pressTime = 0;
        unsigned long releaseTime = 0;
        unsigned long lastAutoClickTime = 0;
        
        bool autoClickEnabled = false;
        bool lateClickEnabled = false;
        uint32_t riseThreshold = 20;
        uint32_t fallThreshold = 10;
        uint32_t longPressThreshold = 550;
        uint32_t autoClickInterval = 500;
    };

    Mux4067stack* _mux;
    Mux4067stack::READINGS* _readings;
    
    std::vector<MuxedEncoderConfig> _muxedEncoderConfigs;
    std::vector<DirectEncoderConfig> _directEncoderConfigs;
    std::vector<MuxedButtonConfig> _muxedButtonConfigs;
    std::vector<DirectButtonConfig> _directButtonConfigs;
    
    std::vector<MuxEncoder> _muxedEncoders;
    std::vector<DirectEncoder> _directEncoders;
    std::vector<MuxButton> _muxedButtons;
    std::vector<DirectButton> _directButtons;
    
    std::function<void(int, int)> _encoderCallback;
    std::function<void(int, MuxButton::btnEvents)> _buttonCallback;
};