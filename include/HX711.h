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

#include <cstdint>
#include <mutex>

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

class HX711 {

protected:

    /**
     * Maximum number of attempts to read bytes from the sensor
     * before failing. Can be interpreted as:
     * 
     * "Will check every _WAIT_INTERVAL_US microseconds to a 
     * maximum of _MAX_READ_TRIES times"
     * 
     * ie. a TimeoutException will occur after at least:
     * _WAIT_INTERVAL_US * _MAX_READ_TRIES microseconds has
     * elapsed
     */
    static const std::uint8_t _MAX_READ_TRIES = 100;
    static const std::uint16_t _WAIT_INTERVAL_US = 1000;

    //Datasheet pg. 5
    //HX711 is a 24-bit ADC (ie. 3 8-bit values = 24 bits)
    static const std::uint8_t _BYTES_PER_CONVERSION_PERIOD = 3;

    /**
     * ints (not int32_t) are used for pins to be as compatible as possible
     * with wiringPi calls (and to not make presumptions about pin 
     * numbering schemes).
     */
    int _gpioHandle;
    const int _dataPin;
    const int _clockPin;
    std::mutex _readLock;
    Gain _gain;
    Format _bitFormat;
    Format _byteFormat;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    bool _readBit() const noexcept;
    std::uint8_t _readByte() const noexcept;
    void _readRawBytes(std::uint8_t* const bytes = nullptr);
    HX_VALUE _readInt();
    static void _delayMicroseconds(const unsigned int us) noexcept;

    HX_VALUE _getChannelAValue();
    HX_VALUE _getChannelBValue();

public:
    
    HX711(const int dataPin, const int clockPin) noexcept;
    virtual ~HX711();

    void begin();

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    void setGain(const Gain gain);
    Gain getGain() const noexcept;
    
    bool isReady() noexcept;

    /**
     * If Channel B value is requested but an exception is thrown
     * setGain MUST be called again.
     */
    HX_VALUE getValue(const Channel c = Channel::A);

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