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

#include "../include/HX711.h"
#include <wiringPi.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

namespace HX711 {

int32_t HX711::_convertFromTwosComplement(const int32_t val) {
    return -(val & 0x800000) + (val & 0x7fffff);
}

bool HX711::_readBit() const {
    digitalWrite(this->_clockPin, HIGH);
    digitalWrite(this->_clockPin, LOW);
    return digitalRead(this->_dataPin) == HIGH;
}

uint8_t HX711::_readByte() const {

    uint8_t val = 0;

    for(uint8_t i = 0; i < 8; ++i) {
        if(this->_bitFormat == Format::MSB) {
            val <<= 1;
            val |= this->_readBit();
        }
        else {
            val >>= 1;
            val |= this->_readBit() * 0x80;
        }
    }

    return val;

}

void HX711::_readRawBytes(uint8_t* bytes) {

    std::unique_lock<std::mutex> lock(this->_readLock);

    while(!this->is_ready()) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }

    const uint8_t byte1 = this->_readByte();
    const uint8_t byte2 = this->_readByte();
    const uint8_t byte3 = this->_readByte();

    for(uint8_t i = 0; i < this->_gain; ++i) {
        this->_readBit();
    }

    lock.unlock();

    if(bytes == nullptr) {
        return;
    }

    if(this->_byteFormat == Format::LSB) {
        bytes[0] = byte3;
        bytes[1] = byte2;
        bytes[2] = byte1;
    }
    else {
        bytes[0] = byte1;
        bytes[1] = byte2;
        bytes[2] = byte3;
    }

}

int32_t HX711::_readLong() {

    uint8_t bytes[3];
    
    this->_readRawBytes(bytes);

    const int32_t twosComp = ((bytes[0] << 16) |
                              (bytes[1] << 8)  |
                              bytes[2]);

    const int32_t signedInt = _convertFromTwosComplement(twosComp);

    return signedInt;

}

double HX711::_readAverage(const uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    else if(times < 5) {
        return this->_readMedian(times);
    }

    std::vector<int32_t> values;
    values.reserve(times);

    for(uint16_t i = 0; i < times; ++i) {
        values.push_back(this->_readLong());
    }

    std::sort(values.begin(), values.end());

    const uint16_t trimAmount = values.size() * 0.2;
    values = std::vector<int32_t>(values.begin() + trimAmount, values.end() - trimAmount);

    const int64_t sum = std::accumulate(values.begin(), values.end(), 0);

    return (double)sum / values.size();

}

double HX711::_readMedian(const uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    else if(times == 1) {
        return (double)this->_readLong();
    }

    std::vector<int32_t> values;
    values.reserve(times);

    for(uint16_t i = 0; i < times; ++i) {
        values.push_back(this->_readLong());
    }

    std::sort(values.begin(), values.end());

    if((times & 0x1) == 0x1) {
        return values[values.size() / 2];
    }

    const uint16_t midpoint = values.size() / 2;
    const int64_t sum = std::accumulate(values.begin() + midpoint, values.begin() + midpoint + 2, 0);

    return sum / 2.0;


}

void HX711::_setOffset(const int32_t offset) {
    this->_setOffsetA(offset);
}

void HX711::_setOffsetA(const int32_t offset) {
    this->_offset = offset;
}

void HX711::_setOffsetB(const int32_t offset) {
    this->_offsetB = offset;
}

int32_t HX711::_getOffset() const {
    return this->_getOffsetA();
}

int32_t HX711::_getOffsetA() const {
    return this->_offset;
}

int32_t HX711::_getOffsetB() const {
    return this->_offsetB;
}

HX711::HX711(const uint8_t dataPin, const uint8_t clockPin, const uint8_t gain) {

    this->_dataPin = dataPin;
    this->_clockPin = clockPin;
    this->_referenceUnit = 1;
    this->_offset = 1;
    this->_byteFormat = Format::MSB;
    this->_bitFormat = Format::MSB;

    wiringPiSetup();
    pinMode(this->_dataPin, INPUT);
    pinMode(this->_clockPin, OUTPUT);

    this->set_gain(gain);

}

HX711::~HX711() {

}

bool HX711::is_ready() const {
    return digitalRead(this->_dataPin) == LOW;
}

void HX711::set_gain(const uint8_t gain) {

    if(gain == 128) {
        this->_gain = 1;
    }
    else if(gain == 64) {
        this->_gain = 3;
    }
    else if(gain == 32) {
        this->_gain = 2;
    }

    digitalWrite(this->_clockPin, LOW);

    this->_readRawBytes();

}

uint8_t HX711::get_gain() const {

    if(this->_gain == 1) {
        return 128;
    }
    else if(this->_gain == 3) {
        return 64;
    }
    else if(this->_gain == 2) {
        return 32;
    }

    return 0;

}

double HX711::get_value(const uint16_t times) {
    return this->get_value_A(times);
}

double HX711::get_value_A(const uint16_t times) {
    return this->_readMedian(times) - this->_getOffsetA();
}

double HX711::get_value_B(const uint16_t times) {
    const uint8_t gain = this->get_gain();
    this->set_gain(32);
    const double val = this->_readMedian(times) - this->_getOffsetB();
    this->set_gain(gain);
    return val;
}

double HX711::get_weight(const uint16_t times) {
    return this->get_weight_A(times);
}

double HX711::get_weight_A(const uint16_t times) {
    double val = this->get_value_A(times);
    val = val / this->_referenceUnit;
    return val;
}

double HX711::get_weight_B(const uint16_t times) {
    double val = this->get_value_B(times);
    val = val / this->_referenceUnitB;
    return val;
}

double HX711::tare(const uint16_t times) {
    return this->tare_A(times);
}

double HX711::tare_A(const uint16_t times) {

    const int32_t backupRefUnit = this->get_reference_unit_A();
    this->set_reference_unit_A(1);

    const double val = this->_readAverage(times);

    this->_setOffsetA(val);
    this->set_reference_unit_A(backupRefUnit);

    return val;

}

double HX711::tare_B(const uint16_t times) {

    const int32_t backupRefUnit = this->get_reference_unit_B();
    this->set_reference_unit_B(1);

    const uint8_t backupGain = this->get_gain();
    this->set_gain(32);

    const double val = this->_readAverage(times);

    this->_setOffsetB(val);

    this->set_gain(backupGain);
    this->set_reference_unit_B(backupRefUnit);

    return val;

}

void HX711::set_reading_format(const Format byteFormat, const Format bitFormat) {
    this->_byteFormat = byteFormat;
    this->_bitFormat = bitFormat;
}

void HX711::set_reference_unit(const int32_t refUnit) {
    this->set_reference_unit_A(refUnit);
}

void HX711::set_reference_unit_A(const int32_t refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_referenceUnit = refUnit;

}

void HX711::set_reference_unit_B(const int32_t refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_referenceUnitB = refUnit;

}

int32_t HX711::get_reference_unit() const {
    return this->get_reference_unit_A();
}

int32_t HX711::get_reference_unit_A() const {
    return this->_referenceUnit;
}

int32_t HX711::get_reference_unit_B() const {
    return this->_referenceUnitB;
}

void HX711::power_down() {

    std::lock_guard<std::mutex> lock(this->_readLock);

    digitalWrite(this->_clockPin, LOW);
    digitalWrite(this->_clockPin, HIGH);

    std::this_thread::sleep_for(std::chrono::microseconds(100));

}

void HX711::power_up() {

    std::unique_lock<std::mutex> lock(this->_readLock);

    digitalWrite(this->_clockPin, LOW);

    std::this_thread::sleep_for(std::chrono::microseconds(100));

    lock.unlock();

    if(this->get_gain() != 128) {
        this->_readRawBytes();
    }

}

void HX711::reset() {
    this->power_down();
    this->power_up();
}

};