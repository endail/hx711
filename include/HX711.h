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

namespace HX711 {

enum class Format {
    MSB,
    LSB
};

class HX711 {

protected:
    std::uint8_t _dataPin;
    std::uint8_t _clockPin;
    std::mutex _readLock;
    std::uint8_t _gain;
    std::int32_t _referenceUnit;
    std::int32_t _referenceUnitB;
    std::int32_t _offset;
    std::int32_t _offsetB;
    Format _byteFormat;
    Format _bitFormat;

    static std::int32_t _convertFromTwosComplement(const std::int32_t val);
    bool _readBit() const;
    std::uint8_t _readByte() const;
    void _readRawBytes(std::uint8_t* bytes = nullptr);
    std::int32_t _readLong();
    double _readAverage(const std::uint16_t times = 3);
    double _readMedian(const std::uint16_t times = 3);

public:
    HX711(const std::uint8_t dataPin, const std::uint8_t clockPin, const std::uint8_t gain = 128);
    ~HX711();
    bool is_ready() const;
    void set_gain(const std::uint8_t gain);
    std::uint8_t get_gain() const;
    double get_value(const std::uint16_t times = 3);
    double get_value_A(const std::uint16_t times = 3);
    double get_value_B(const std::uint16_t times = 3);
    double get_weight(const std::uint16_t times = 3);
    double get_weight_A(const std::uint16_t times = 3);
    double get_weight_B(const std::uint16_t times = 3);
    double tare(const std::uint16_t times = 15);
    double tare_A(const std::uint16_t times = 15);
    double tare_B(const std::uint16_t times = 15);
    void set_reading_format(const Format byteFormat = Format::MSB, const Format bitFormat = Format::MSB);
    void set_reference_unit(const std::int32_t refUnit);
    void set_reference_unit_A(const std::int32_t refUnit);
    void set_reference_unit_B(const std::int32_t refUnit);
    std::int32_t get_reference_unit() const;
    std::int32_t get_reference_unit_A() const;
    std::int32_t get_reference_unit_B() const;
    void setOffset(const std::int32_t offset);
    void setOffsetA(const std::int32_t offset);
    void setOffsetB(const std::int32_t offset);
    std::int32_t getOffset() const;
    std::int32_t getOffsetA() const;
    std::int32_t getOffsetB() const;
    void power_down();
    void power_up();
    void reset();

};
};
#endif