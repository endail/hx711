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
#include <vector>
#include <lgpio.h>

namespace HX711 {

/**
 * Datasheet
 * https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 */

typedef std::uint8_t BYTE;

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

enum class PinWatchState {
    NONE = 0,
    NORMAL,
    PAUSE,
    END
};

enum class Rate {
    HZ_10,
    HZ_80
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

class Value {
public:
    operator int32_t() const noexcept;
    bool isSaturated() const noexcept;
    bool isValid() const noexcept;
    Value(const std::int32_t v = _MIN) noexcept;
    Value& operator=(const Value& v2) noexcept;

protected:
    int32_t _v;

    /**
     * Datasheet pg. 3
     */
    static const int32_t _MIN = -0x800000;
    static const int32_t _MAX = 0x7FFFFF;

};

struct Timing {
    std::chrono::high_resolution_clock::time_point begin;
    std::chrono::high_resolution_clock::time_point ready;
    std::chrono::high_resolution_clock::time_point end;
    std::chrono::high_resolution_clock::time_point nextbegin;

    std::chrono::microseconds getTimeToReady() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->ready - this->begin);
    }

    std::chrono::microseconds getTimeToConvert() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->end - this->ready);
    }

    std::chrono::microseconds getTimeBetweenConversions() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->nextbegin - this->end);
    }

    std::chrono::microseconds getTotalTime() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->ready - this->begin);
    }

};

class HX711 {

protected:

    static const std::uint8_t _BYTES_PER_CONVERSION_PERIOD = 3;

    static constexpr std::chrono::nanoseconds _DEFAULT_MAX_WAIT =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::seconds(1));

    static constexpr std::chrono::nanoseconds _DEFAULT_PAUSE_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(100));

    static constexpr std::chrono::nanoseconds _DEFAULT_POLL_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(57));

    static constexpr std::chrono::nanoseconds _DEFAULT_NOT_READY_SLEEP =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::microseconds(7));

    int _gpioHandle;
    const int _dataPin;
    const int _clockPin;
    std::mutex _commLock;
    std::mutex _readyLock;
    std::mutex _pinWatchLock;
    std::condition_variable _dataReady;
    Value _lastVal;
    PinWatchState _watchState;
    std::chrono::nanoseconds _pauseSleep;
    std::chrono::nanoseconds _notReadySleep;
    std::chrono::nanoseconds _pollSleep;
    Rate _rate;
    Channel _channel;
    Gain _gain;
    Format _bitFormat;
    Format _byteFormat;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    static std::uint8_t _calculatePulses(const Gain g) noexcept;
    bool _isReady() noexcept;
    bool _readBit() noexcept;
    BYTE _readByte() noexcept;
    void _readRawBytes(BYTE* bytes = nullptr);
    Value _readInt();
    static void _sleepns(const std::chrono::nanoseconds ns) noexcept;
    static void _delayns(const std::chrono::nanoseconds ns) noexcept;
    void _watchPin();
    void _changeWatchState(const PinWatchState state);

public:
    
    HX711(const int dataPin, const int clockPin) noexcept;
    virtual ~HX711();

    void begin();

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    Channel getChannel() const noexcept;
    Gain getGain() const noexcept;
    void setConfig(const Channel c = Channel::A, const Gain g = Gain::GAIN_128);

    std::vector<Timing> testTiming(const std::size_t samples = 1000) noexcept;

    Value getValue();
    void getValues(
        Value* const arr,
        const std::size_t len,
        const std::chrono::nanoseconds maxWait = _DEFAULT_MAX_WAIT);

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