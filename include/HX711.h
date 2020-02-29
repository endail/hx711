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

#ifndef _HX711_H_
#define _HX711_H_

#include <stdint.h>
#include <mutex>

namespace HX711 {

enum Format {
    MSB,
    LSB
};

class HX711 {

protected:
    uint8_t _dataPin;
    uint8_t _clockPin;
    std::mutex _readLock;
    uint8_t _gain;
    int32_t _referenceUnit;
    int32_t _referenceUnitB;
    int32_t _offset;
    int32_t _offsetB;
    Format _byteFormat;
    Format _bitFormat;

    static int32_t _convertFromTwosComplement(const int32_t val);
    bool _readBit() const;
    uint8_t _readByte() const;
    void _readRawBytes(uint8_t* bytes = nullptr);
    int32_t _readLong();
    double _readAverage(const uint8_t times = 3);
    double _readMedian(const uint8_t times = 3);
    void _setOffset(const int32_t offset);
    void _setOffsetA(const int32_t offset);
    void _setOffsetB(const int32_t offset);
    int32_t _getOffset() const;
    int32_t _getOffsetA() const;
    int32_t _getOffsetB() const;

public:
    HX711(const uint8_t dataPin, const uint8_t clockPin, const uint8_t gain = 128);
    ~HX711();
    bool is_ready() const;
    void set_gain(const uint8_t gain);
    uint8_t get_gain() const;
    double get_value(const uint8_t times = 3);
    double get_value_A(const uint8_t times = 3);
    double get_value_B(const uint8_t times = 3);
    double get_weight(const uint8_t times = 3);
    double get_weight_A(const uint8_t times = 3);
    double get_weight_B(const uint8_t times = 3);
    double tare(const uint8_t times = 15);
    double tare_A(const uint8_t times = 15);
    double tare_B(const uint8_t times = 15);
    void set_reading_format(const Format byteFormat = Format::MSB, const Format bitFormat = Format::MSB);
    void set_reference_unit(const int32_t refUnit);
    void set_reference_unit_A(const int32_t refUnit);
    void set_reference_unit_B(const int32_t refUnit);
    int32_t get_reference_unit() const;
    int32_t get_reference_unit_A() const;
    int32_t get_reference_unit_B() const;
    void power_down();
    void power_up();
    void reset();

};
};
#endif