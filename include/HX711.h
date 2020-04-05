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

//C++ port of https://github.com/tatobari/hx711py

#ifndef HX711_HX711_H_670BFDCD_DA15_4F8B_A15C_0F0043905889
#define HX711_HX711_H_670BFDCD_DA15_4F8B_A15C_0F0043905889

#include <cstdint>
#include <mutex>
#include <vector>

namespace HX711 {

enum class Format {
    MSB,
    LSB
};

enum class Gain {
    GAIN_128 = 0,
    GAIN_32,
    GAIN_64
};

const std::uint8_t PULSES[3] = {
    25,
    26,
    27
};

class HX711 {

protected:
    std::uint8_t _dataPin;
    std::uint8_t _clockPin;
    std::mutex _readLock;
    Gain _gain;
    std::int32_t _referenceUnit;
    std::int32_t _referenceUnitB;
    std::int32_t _offset;
    std::int32_t _offsetB;
    Format _byteFormat;
    Format _bitFormat;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val) noexcept;
    bool _readBit() const noexcept;
    std::uint8_t _readByte() const noexcept;
    void _readRawBytes(std::uint8_t* bytes = nullptr) noexcept;
    std::int32_t _readLong() noexcept;

public:
    HX711(
        const std::uint8_t dataPin,
        const std::uint8_t clockPin,
        const Gain gain = Gain::GAIN_128);

    virtual ~HX711() = default;
    std::uint8_t getDataPin() const noexcept;
    std::uint8_t getClockPin() const noexcept;
    bool is_ready() const noexcept;
    void set_gain(const Gain gain) noexcept;
    Gain get_gain() const noexcept;
    double get_value(const std::uint16_t times = 3) noexcept;
    double get_value_A(const std::uint16_t times = 3) noexcept;
    double get_value_B(const std::uint16_t times = 3) noexcept;
    double get_weight(const std::uint16_t times = 3) noexcept;
    std::vector<double> get_weights(const std::uint16_t times = 3);
    double get_weight_A(const std::uint16_t times = 3) noexcept;
    double get_weight_B(const std::uint16_t times = 3) noexcept;
    double tare(const std::uint16_t times = 15) noexcept;
    double tare_A(const std::uint16_t times = 15) noexcept;
    double tare_B(const std::uint16_t times = 15) noexcept;
    void set_reading_format(
        const Format byteFormat = Format::MSB,
        const Format bitFormat = Format::MSB) noexcept;
    void set_reference_unit(const std::int32_t refUnit);
    void set_reference_unit_A(const std::int32_t refUnit);
    void set_reference_unit_B(const std::int32_t refUnit);
    std::int32_t get_reference_unit() const noexcept;
    std::int32_t get_reference_unit_A() const noexcept;
    std::int32_t get_reference_unit_B() const noexcept;
    void setOffset(const std::int32_t offset) noexcept;
    void setOffsetA(const std::int32_t offset) noexcept;
    void setOffsetB(const std::int32_t offset) noexcept;
    std::int32_t getOffset() const noexcept;
    std::int32_t getOffsetA() const noexcept;
    std::int32_t getOffsetB() const noexcept;
    double readAverage(const std::uint16_t times = 3);
    double readMedian(const std::uint16_t times = 3);
    void power_down() noexcept;
    void power_up() noexcept;
    void reset() noexcept;

};
};
#endif