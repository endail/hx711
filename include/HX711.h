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

#include <bitset>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include "Value.h"

namespace HX711 {

/**
 * Datasheet
 * https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 */

enum class Channel {
    A,
    B
};

/**
 * Datasheet pg. 4
 */
enum class Gain {
    GAIN_128,
    GAIN_32,
    GAIN_64
};

/**
 * Datasheet pg. 3
 * OTHER is to be used when an external clock (ie. crystal is used)
 */
enum class Rate {
    HZ_10,
    HZ_80,
    OTHER
};

class HX711 {
protected:
    
    static const std::uint8_t _BITS_PER_CONVERSION_PERIOD = 24;
    static const std::unordered_map<const Gain, const std::uint8_t> _PULSES;
    static constexpr auto _T1 = std::chrono::nanoseconds(100);
    static constexpr auto _T2 = std::chrono::nanoseconds(100);
    static constexpr auto _T3 = std::chrono::nanoseconds(200);
    static constexpr auto _T4 = std::chrono::nanoseconds(200);
    static constexpr auto _POWER_DOWN_TIMEOUT = std::chrono::microseconds(60);
    static const std::unordered_map<const Rate, const std::chrono::nanoseconds> _SETTLING_TIMES;

    int _gpioHandle;
    const int _dataPin;
    const int _clockPin;
    std::mutex _commLock;
    Rate _rate;
    Channel _channel;
    Gain _gain;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    static std::uint8_t _calculatePulses(const Gain g) noexcept;
    void _setInputGainSelection();
    bool _readBit() const;
    void _readBits(std::int32_t* const v);


public:
    
    HX711(
        const int dataPin,
        const int clockPin,
        const Rate rate = Rate::HZ_10) noexcept;

    HX711(const HX711& that) = delete;
    HX711& operator=(const HX711& that) = delete;

    virtual ~HX711();

    void begin();

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    Channel getChannel() const noexcept;
    Gain getGain() const noexcept;
    void setConfig(const Channel c = Channel::A, const Gain g = Gain::GAIN_128);

    bool isReady() const;
    Value readValue();

    void powerDown();
    void powerUp();

};
};

#endif
