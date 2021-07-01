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
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <lgpio.h>

#include <vector>

namespace HX711 {

/**
 * Datasheet
 * https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 */

/**
 * The HX711 is a 24-bit ADC. Values it outputs will always be
 * treated as 32-bit integers and not floating point numbers.
 */
typedef std::int32_t HX_VALUE;

enum class Format {
    MSB = 0, //most significant bit
    LSB //least significant bit
};

enum class Channel {
    A = 0,
    B
};

//Datasheet pg. 4
enum class Gain {
    GAIN_128 = 0,
    GAIN_32,
    GAIN_64
};

/**
 * Used as a map to select to correct number of clock pulses
 * depending on the set gain
 * Datasheet pg. 4
 */
const std::uint8_t PULSES[3] = {
    25,
    26,
    27
};

struct Timing {
    std::chrono::high_resolution_clock::time_point begin;
    std::chrono::high_resolution_clock::time_point ready;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::high_resolution_clock::time_point nextbegin;
    std::chrono::microseconds getDiff() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->nextbegin - this->end);
    }
};

class HX711 {

protected:

    static constexpr std::chrono::nanoseconds _DEFAULT_MAX_WAIT =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::seconds(1));

    static const std::uint8_t _BYTES_PER_CONVERSION_PERIOD = 3;

    static const HX_VALUE HX_MIN_VALUE = 0x800000;
    static const HX_VALUE HX_MAX_VALUE = 0x7FFFFF;

    static constexpr std::chrono::nanoseconds _DEFAULT_POLL_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(1));

    static constexpr std::chrono::nanoseconds _DEFAULT_SATURATED_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(1));

    static constexpr std::chrono::nanoseconds _DEFAULT_NOT_READY_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(1));

    int _gpioHandle;
    const int _dataPin;
    const int _clockPin;
    std::mutex _commLock;
    std::mutex _readyLock;
    std::condition_variable _dataReady;
    std::chrono::nanoseconds _maxWait;
    HX_VALUE _lastVal;
    bool _pollPin;
    std::chrono::nanoseconds _notReadySleep;
    std::chrono::nanoseconds _saturatedSleep;
    std::chrono::nanoseconds _pollSleep;
    Channel _channel;
    Gain _gain;
    Format _bitFormat;
    Format _byteFormat;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    static bool _isSaturated(const HX_VALUE v);
    bool _readBit() noexcept;
    std::uint8_t _readByte() noexcept;
    void _readRawBytes(std::uint8_t* bytes = nullptr);
    HX_VALUE _readInt();
    static void _delayMicroseconds(const unsigned int us) noexcept;
    void _watchPin() noexcept;

public:
    
    HX711(const int dataPin, const int clockPin) noexcept;
    virtual ~HX711();

    void begin();

    void setMaxWaitTime(
        const std::chrono::nanoseconds maxWait = _DEFAULT_MAX_WAIT) noexcept;

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    Channel getChannel() const noexcept;
    Gain getGain() const noexcept;
    void setConfig(const Channel c = Channel::A, const Gain g = Gain::GAIN_128);

    bool isReady() noexcept;

    std::vector<Timing> testTiming(const size_t samples = 1000) noexcept;

    HX_VALUE getValue();

    Format getBitFormat() const noexcept;
    Format getByteFormat() const noexcept;
    void setBitFormat(const Format f) noexcept;
    void setByteFormat(const Format f) noexcept;

    void powerDown() noexcept;
    void powerUp();

};
};

#include "Mass.h"
#include "TimeoutException.h"
#include "SimpleHX711.h"

#endif