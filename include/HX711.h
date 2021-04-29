// MIT License
//
// Copyright (c) 2020 Daniel Robertson
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

namespace HX711 {

/**
 * Datasheet
 * https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
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
    GAIN_128 = 0,
    GAIN_32,
    GAIN_64
};

//Datasheet pg. 4
const std::uint8_t PULSES[3] = {
    25,
    26,
    27
};

class HX711 {

protected:

    /**
     * Maximum number of attempts to read bytes from the sensor
     * before failing
     */
    static const std::uint8_t _MAX_READ_TRIES = 3;
    static constexpr std::chrono::microseconds _WAIT_INTERVAL = std::chrono::microseconds(1);

    //Datasheet pg. 5
    static const std::uint8_t _BYTES_PER_CONVERSION_PERIOD = 3;

    /**
     * ints (not int32_t) are used for pins to be as compatible as possible
     * with wiringPi calls (and to not make presumptions about pin 
     * numbering schemes).
     */
    const int _dataPin = -1;
    const int _clockPin = -1;
    std::mutex _readLock;
    Gain _gain = Gain::GAIN_128;
    Format _bitFormat = Format::MSB;
    Format _byteFormat = Format::MSB;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    bool _readBit() const noexcept;
    std::uint8_t _readByte() const noexcept;
    void _readRawBytes(std::uint8_t* bytes = nullptr);
    std::int32_t _readInt();

    std::int32_t _getChannelAValue();
    std::int32_t _getChannelBValue();

public:
    
    HX711(const int dataPin, const int clockPin) noexcept;
    virtual ~HX711() = default;

    int getDataPin() const noexcept;
    int getClockPin() const noexcept;

    void setGain(const Gain gain);
    Gain getGain() const noexcept;

    void connect(
        const Gain gain = Gain::GAIN_128,
        const Format bitFormat = Format::MSB,
        const Format byteFormat = Format::MSB);
    
    bool isReady() const noexcept;

    std::int32_t getValue(const Channel c = Channel::A);

    void setReadFormat(
        const Format bitFormat = Format::MSB,
        const Format byteFormat = Format::MSB) noexcept;

    void powerDown() noexcept;
    void powerUp();

};
};
#endif