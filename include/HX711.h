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
#include <unordered_map>
#include <lgpio.h>
#include <sched.h>

namespace HX711 {

/**
 * Datasheet
 * https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 */

typedef std::uint8_t BYTE;

/**
 * MSB - most significant bit
 * LSB - least significant bit
 */
enum class Format {
    MSB,
    LSB
};

enum class Channel {
    A,
    B
};

//Datasheet pg. 4
enum class Gain {
    GAIN_128,
    GAIN_32,
    GAIN_64
};

enum class PinWatchState {
    NONE,
    NORMAL,
    PAUSE,
    END
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

enum class GpioLevel {
    LOW = 0,
    HIGH = 1
};

class Value {

protected:

    typedef std::int32_t _INTERNAL_TYPE;

    _INTERNAL_TYPE _v;

    /**
     * Datasheet pg. 3
     * But also a consequence of the sensor being 24 bits
     */
    static const _INTERNAL_TYPE _MIN = -0x800000;
    static const _INTERNAL_TYPE _MAX = 0x7FFFFF;

public:

    /**
     * When input differential signal goes out of
     * the 24 bit range, the output data will be saturated
     * at 800000h (MIN) or 7FFFFFh (MAX), until the
     * input signal comes back to the input range.
     * Datasheet pg. 4
     */
    bool isSaturated() const noexcept;

    /**
     * A 32 bit integer holds the actual value from the sensor. Calling
     * this function makes sure it is within the 24 bit range used by
     * the sensor.
     */
    bool isValid() const noexcept;
    operator _INTERNAL_TYPE() const noexcept;
    
    //cppcheck-suppress noExplicitConstructor
    Value(const _INTERNAL_TYPE v) noexcept;
    Value() noexcept;
    Value& operator=(const Value& v2) noexcept;

};

class HX711 {

protected:
    
    static const std::unordered_map<const Gain, const std::uint8_t> _PULSES;
    static const std::unordered_map<const Rate, const std::chrono::nanoseconds> _SETTLING_TIMES;
    
    static const std::uint8_t _BYTES_PER_CONVERSION_PERIOD = 3;
    static const int _PINWATCH_SCHED_POLICY = SCHED_FIFO;

    static constexpr std::chrono::nanoseconds _T1 = std::chrono::nanoseconds(100);
    static constexpr std::chrono::nanoseconds _T2 = std::chrono::nanoseconds(100);
    static constexpr std::chrono::nanoseconds _T3 = std::chrono::nanoseconds(200);
    static constexpr std::chrono::nanoseconds _T4 = std::chrono::nanoseconds(200);
    
    static constexpr std::chrono::microseconds _POWER_DOWN_TIMEOUT =
        std::chrono::microseconds(60);

    static constexpr std::chrono::microseconds _DEFAULT_MAX_WAIT =
        std::chrono::duration_cast<std::chrono::microseconds>(
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
    GpioLevel _readGpio(const int pin);
    void _writeGpio(const int pin, const GpioLevel lev);
    bool _isReady();
    bool _readBit();
    BYTE _readByte();
    void _readRawBytes(BYTE* const bytes = nullptr);
    Value _readInt();

    /**
     * Sleep for ns nanoseconds. The _sleepns/_delayns functions are
     * an attempt to be analogous to usleep/udelay in the kernel.
     * https://www.kernel.org/doc/html/v5.10/timers/timers-howto.html
     */
    static void _sleepns(const std::chrono::nanoseconds ns) noexcept;
    static void _sleepus(const std::chrono::microseconds us) noexcept;
    
    /**
     * Delay for ns nanoseconds. The _sleepns/_delayns functions are
     * an attempt to be analogous to usleep/udelay in the kernel.
     * https://www.kernel.org/doc/html/v5.10/timers/timers-howto.html
     */
    static void _delayns(const std::chrono::nanoseconds ns) noexcept;
    static void _delayus(const std::chrono::microseconds us) noexcept;
    
    static void* _watchPin(void* const hx711ptr);
    void _changeWatchState(const PinWatchState state);

public:
    
    HX711(const int dataPin, const int clockPin) noexcept;
    virtual ~HX711();

    void begin();

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    Channel getChannel() const noexcept;
    Gain getGain() const noexcept;
    void setConfig(
        const Channel c = Channel::A,
        const Gain g = Gain::GAIN_128,
        const Rate r = Rate::HZ_10);

    Value getValue();
    void getValues(
        Value* const arr,
        const std::size_t len,
        const std::chrono::microseconds maxWait = _DEFAULT_MAX_WAIT);

    Format getBitFormat() const noexcept;
    Format getByteFormat() const noexcept;
    void setFormat(const Format bitF, const Format byteF) noexcept;

    void powerDown();
    void powerUp();

};
};

#include "Mass.h"
#include "GpioException.h"
#include "TimeoutException.h"
#include "SimpleHX711.h"
#include "Discovery.h"

#endif