// MIT License
//
// Copyright (c) 2021 Daniel Robertson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HX711_HX711_H_670BFDCD_DA15_4F8B_A15C_0F0043905889
#define HX711_HX711_H_670BFDCD_DA15_4F8B_A15C_0F0043905889

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace HX711 {

/**
 * Datasheet pg. 4
 */
enum class Gain : unsigned int {
    GAIN_128,
    GAIN_32,
    GAIN_64
};

/**
 * Datasheet pg. 3
 * OTHER is to be used when an external clock (ie. crystal) is used
 */
enum class Rate : unsigned int {
    HZ_10,
    HZ_80,
    OTHER
};

class HX711 {
protected:

    static const std::int32_t _MIN_VALUE = -0x800000;
    static const std::int32_t _MAX_VALUE = 0x7fffff;
    static const unsigned int _BITS_PER_CONVERSION_PERIOD = 24;
    static const std::unordered_map<const Gain, const unsigned char> _PULSES;
    static constexpr auto _T1 = std::chrono::nanoseconds(100);
    static constexpr auto _T2 = std::chrono::nanoseconds(100);
    static constexpr auto _T3 = std::chrono::nanoseconds(200);
    static constexpr auto _T4 = std::chrono::nanoseconds(200);
    static constexpr auto _POWER_DOWN_TIMEOUT = std::chrono::microseconds(60);
    static const std::unordered_map<const Rate, const std::chrono::milliseconds> _SETTLING_TIMES;

    int _gpioHandle;
    const int _dataPin;
    const int _clockPin;
    const Rate _rate;
    std::mutex _commLock;
    Gain _gain;
    bool _strictTiming;
    bool _useDelays;

    static inline std::int32_t HX711::_convertFromTwosComplement(const std::uint32_t val) noexcept {
        return (std::int32_t)(-(raw & +_MIN_VALUE)) + (std::int32_t)(raw & _MAX_VALUE);
    }

    static inline unsigned int _calculatePulses(const Gain g) noexcept {
        return _PULSES.at(g) - _BITS_PER_CONVERSION_PERIOD;
    }

    inline void HX711::_setInputGainSelection() {

        const auto pulses = _calculatePulses(this->_gain);

        for(auto i = decltype(pulses){0}; i < pulses; ++i) {
            this->_pulseClockNoRead();
        }

    }

    void _pulseClockNoRead();
    bool _readBit() const;
    void _readBits(std::int32_t* const v);


public:

    static inline bool isMinSaturated(const std::int32_t v) noexcept {
        return v == _MIN_VALUE;
    }

    static inline bool isMaxSaturated(const std::int32_t v) noexcept {
        return v == _MAX_VALUE;
    }

    HX711(
        const int dataPin,
        const int clockPin,
        const Rate rate = Rate::HZ_10) noexcept;

    HX711(const HX711& that) = delete;
    HX711& operator=(const HX711& that) = delete;

    inline virtual ~HX711() {
        try {
            this->close();
        }
        catch(...) {
            //do not allow propagation
        }
    }

    void init();
    void close();

    inline void HX711::setStrictTiming(const bool strict) noexcept {
        std::lock_guard<std::mutex> lock(this->_commLock);
        this->_strictTiming = strict;
    }

    inline bool HX711::isStrictTiming() const noexcept {
        return this->_strictTiming;
    }

    inline void HX711::useDelays(const bool use) noexcept {
        this->_useDelays = use;
    }

    inline bool HX711::isUsingDelays() const noexcept {
        return this->_useDelays;
    }

    inline int HX711::getDataPin() const noexcept {
        return this->_dataPin;
    }

    inline int HX711::getClockPin() const noexcept {
        return this->_clockPin;
    }

    inline Gain HX711::getGain() const noexcept {
        return this->_gain;
    }

    void setGain(const Gain g);

    bool isReady() const;
    virtual bool waitReady(const std::chrono::nanoseconds timeout = std::chrono::seconds(1)) const;
    std::int32_t readValue();

    inline void HX711::powerDown() {
        std::lock_guard<std::mutex> lock(this->_commLock);
        Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::HIGH);
    }

    inline void HX711::powerUp() {
        std::lock_guard<std::mutex> lock(this->_commLock);
        Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);
    }

};
};

#endif
