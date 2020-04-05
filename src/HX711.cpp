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
#include <cstdint>
#include <wiringPi.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstring>

namespace HX711 {

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    return -(val & 0x800000) + (val & 0x7fffff);
}

bool HX711::_readBit() const noexcept {

    /**
     *  A new bit will be "ready" when the clock pin
     *  is held high for 1us, then low for 1us.
     *  There is no subsequent delay for reading the bit.
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     * 
     *  Problem 1: the max high time for holding clock pin
     *  high is 50us. Typically (according to docs) 1us is
     *  sufficient.
     * 
     *  Solution: stick with 1us.
     */

    digitalWrite(this->_clockPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(this->_clockPin, LOW);
    delayMicroseconds(1);

    return digitalRead(this->_dataPin) == HIGH;

}

std::uint8_t HX711::_readByte() const noexcept {

    std::uint8_t val = 0;

    /**
     *  ISSUE: does there need to be a delay between
     *  reading each bit? Docs describe a 0.1us delay
     *  prior to reading the most significant bit.
     *  But it is unclear if this delay applies to
     *  each bit.
     */

    for(std::uint8_t i = 0; i < 8; ++i) {
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

void HX711::_readRawBytes(std::uint8_t* bytes) noexcept {

    std::unique_lock<std::mutex> lock(this->_readLock);

    while(!this->is_ready()) {
        /**
         *  HX711 will be "ready" when DOUT is low.
         *  The time between reading a bit and being ready
         *  is a maximum of 0.1us. T2 in Fig.2
         * 
         *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
         *  pg. 5
         *  
         *  Problem 1: wiringPi's delay resolution is to the microsecond,
         *  not nanosecond. A 1us delay here would be 10x the maximum
         *  expected wait period.
         *  https://github.com/WiringPi/WiringPi/blob/master/wiringPi/wiringPi.c#L2143-L2172
         * 
         *  Problem 2: according to Gordon, nanosleep is unreliable. Hence,
         *  we prefer wiringPi's delay functions instead.
         *  https://projects.drogon.net/accurate-delays-on-the-raspberry-pi/
         * 
         *  Solution: no *programmed* delays.
         * 
         *  Any attempt to sleep for such a short amount of time is going to
         *  far exceed the maximum documented wait time for the HX711 anyway. 
         */
    }

    std::uint8_t raw[3] = {
        this->_readByte(),
        this->_readByte(),
        this->_readByte()
    };

    const uint8_t gainLeftover = 
        PULSES[static_cast<std::int32_t>(this->_gain)] - sizeof(raw) * 8;

    for(std::uint8_t i = 0; i < gainLeftover; ++i) {
        this->_readBit();
    }

    lock.unlock();

    if(bytes == nullptr) {
        return;
    }

    /**
     *  The HX711 will supply bits in big-endian format;
     *  the 0th read bit is the MSB.
     *  
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 4
     * 
     *  If LSB format is requested, all that needs to be
     *  done is for the bytes to be reversed.
     */
    if(this->_byteFormat == Format::LSB) {
        std::reverse(raw, raw + sizeof(raw));
    }

    //finally, copy the local raw bytes to the byte array
    memcpy(bytes, raw, sizeof(raw));

}

std::int32_t HX711::_readLong() noexcept {

    std::uint8_t bytes[3];
    
    this->_readRawBytes(bytes);

    const std::int32_t twosComp = ((bytes[0] << 16) |
                                   (bytes[1] << 8)  |
                                    bytes[2]);

    const std::int32_t signedInt = _convertFromTwosComplement(twosComp);

    return signedInt;

}

HX711::HX711(
    const std::uint8_t dataPin,
    const std::uint8_t clockPin,
    const Gain gain)
        :   _dataPin(dataPin),
            _clockPin(clockPin),
            _referenceUnit(1),
            _offset(1),
            _byteFormat(Format::MSB),
            _bitFormat(Format::MSB) {

                wiringPiSetup();
                pinMode(this->_dataPin, INPUT);
                pinMode(this->_clockPin, OUTPUT);

                this->set_gain(gain);

}

std::uint8_t HX711::getDataPin() const noexcept {
    return this->_dataPin;
}

std::uint8_t HX711::getClockPin() const noexcept {
    return this->_clockPin;
}

bool HX711::is_ready() const noexcept {
    /**
     *  HX711 will be "ready" when DOUT is low.
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     */
    return digitalRead(this->_dataPin) == LOW;
}

void HX711::set_gain(const Gain gain) noexcept {
    this->_gain = gain;
    digitalWrite(this->_clockPin, LOW);
    this->_readRawBytes();
}

Gain HX711::get_gain() const noexcept {
    return this->_gain;
}

double HX711::get_value(const std::uint16_t times) noexcept {
    return this->get_value_A(times);
}

double HX711::get_value_A(const std::uint16_t times) noexcept {
    return this->readMedian(times) - this->getOffsetA();
}

double HX711::get_value_B(const std::uint16_t times) noexcept {
    const Gain gain = this->_gain;
    this->_gain = Gain::GAIN_32;
    const double val = this->readMedian(times) - this->getOffsetB();
    this->set_gain(gain);
    return val;
}

double HX711::get_weight(const std::uint16_t times) noexcept {
    return this->get_weight_A(times);
}

double HX711::get_weight_A(const std::uint16_t times) noexcept {
    double val = this->get_value_A(times);
    val = val / this->_referenceUnit;
    return val;
}

double HX711::get_weight_B(const std::uint16_t times) noexcept {
    double val = this->get_value_B(times);
    val = val / this->_referenceUnitB;
    return val;
}

double HX711::tare(const std::uint16_t times) noexcept {
    return this->tare_A(times);
}

double HX711::tare_A(const std::uint16_t times) noexcept {

    const std::int32_t backupRefUnit = this->get_reference_unit_A();
    this->set_reference_unit_A(1);

    const double val = this->readAverage(times);

    this->setOffsetA(val);
    this->set_reference_unit_A(backupRefUnit);

    return val;

}

double HX711::tare_B(const std::uint16_t times) noexcept {

    const std::int32_t backupRefUnit = this->get_reference_unit_B();
    this->set_reference_unit_B(1);

    const Gain backupGain = this->_gain;
    this->set_gain(Gain::GAIN_32);

    const double val = this->readAverage(times);

    this->setOffsetB(val);
    this->set_gain(backupGain);
    this->set_reference_unit_B(backupRefUnit);

    return val;

}

void HX711::set_reading_format(const Format byteFormat, const Format bitFormat) noexcept {
    this->_byteFormat = byteFormat;
    this->_bitFormat = bitFormat;
}

void HX711::set_reference_unit(const std::int32_t refUnit) {
    this->set_reference_unit_A(refUnit);
}

void HX711::set_reference_unit_A(const std::int32_t refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_referenceUnit = refUnit;

}

void HX711::set_reference_unit_B(const std::int32_t refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_referenceUnitB = refUnit;

}

std::int32_t HX711::get_reference_unit() const noexcept {
    return this->get_reference_unit_A();
}

std::int32_t HX711::get_reference_unit_A() const noexcept {
    return this->_referenceUnit;
}

std::int32_t HX711::get_reference_unit_B() const noexcept {
    return this->_referenceUnitB;
}

void HX711::setOffset(const std::int32_t offset) noexcept {
    this->setOffsetA(offset);
}

void HX711::setOffsetA(const std::int32_t offset) noexcept {
    this->_offset = offset;
}

void HX711::setOffsetB(const std::int32_t offset) noexcept {
    this->_offsetB = offset;
}

std::int32_t HX711::getOffset() const noexcept {
    return this->getOffsetA();
}

std::int32_t HX711::getOffsetA() const noexcept {
    return this->_offset;
}

std::int32_t HX711::getOffsetB() const noexcept {
    return this->_offsetB;
}

double HX711::readAverage(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    else if(times < 5) {
        return this->readMedian(times);
    }

    std::vector<std::int32_t> values;
    values.reserve(times);

    for(std::uint16_t i = 0; i < times; ++i) {
        values.push_back(this->_readLong());
    }

    std::sort(values.begin(), values.end());

    const std::uint16_t trimAmount = values.size() * 0.2;
    values = std::vector<std::int32_t>(values.begin() + trimAmount, values.end() - trimAmount);

    const std::int64_t sum = std::accumulate(values.begin(), values.end(), 0);

    return (double)sum / values.size();

}

double HX711::readMedian(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    else if(times == 1) {
        return (double)this->_readLong();
    }

    std::vector<std::int32_t> values;
    values.reserve(times);

    for(std::uint16_t i = 0; i < times; ++i) {
        values.push_back(this->_readLong());
    }

    std::sort(values.begin(), values.end());

    if((times & 0x1) == 0x1) {
        return values[values.size() / 2];
    }

    const std::uint16_t midpoint = values.size() / 2;
    const std::int64_t sum = std::accumulate(values.begin() + midpoint, values.begin() + midpoint + 2, 0);

    return sum / 2.0;

}

void HX711::power_down() noexcept {

    std::lock_guard<std::mutex> lock(this->_readLock);

    digitalWrite(this->_clockPin, LOW);
    digitalWrite(this->_clockPin, HIGH);

    /**
     *  "When PD_SCK pin changes from low to high
     *  and stays at high for longer than 60µs, HX711
     *  enters power down mode (Fig.3)."
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     */
    delayMicroseconds(60);

}

void HX711::power_up() noexcept {

    std::unique_lock<std::mutex> lock(this->_readLock);

    digitalWrite(this->_clockPin, LOW);

    /**
     *  "When PD_SCK returns to low,
     *  chip will reset and enter normal operation mode"
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5   
     */

    lock.unlock();

    if(this->_gain != Gain::GAIN_128) {
        this->_readRawBytes();
    }

}

void HX711::reset() noexcept {
    this->power_down();
    this->power_up();
}

};