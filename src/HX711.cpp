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
     *  is held high for 1us (T3), then low for 1us (T4).
     *  There is no subsequent delay for reading the bit.
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     * 
     *  Problem 1: the max high time for holding clock pin
     *  high is 50us. Typically (according to docs) 1us is
     *  sufficient.
     * 
     *  Solution: stick with 1us. It seems to work fine.
     */
    ::digitalWrite(this->_clockPin, HIGH);
    ::delayMicroseconds(1);
    ::digitalWrite(this->_clockPin, LOW);
    ::delayMicroseconds(1);

    return ::digitalRead(this->_dataPin) == HIGH;

}

std::uint8_t HX711::_readByte() const noexcept {

    std::uint8_t val = 0;

    for(std::uint8_t i = 0; i < _BITS_PER_BYTE; ++i) {
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

    /**
     *  Bytes are ready to be read from the HX711 when DOUT goes low. Therefore,
     *  wait until this occurs.
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     * 
     *  ISSUE: this is essentially an infinite-loop waiting on a GPIO pin. This
     *  may be problematic.
     */
    while(!this->is_ready());

    /**
     *  When DOUT goes low, there is a minimum of 0.1us until the clock pin
     *  can go high. T1 in Fig.2.
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     * 
     *  Problem 1: because we prefer wiringPi's timing functions, we cannot
     *  wait less than 1us.
     * 
     *  Solution: wait for 1us. This is 10x longer than necessary, but it
     *  does allow sufficient time.
     */
    ::delayMicroseconds(1);

    //delcare array of bytes of sufficient size
    std::uint8_t raw[_BYTES_PER_CONVERSION_PERIOD];

    //then populate it with values from the hx711
    for(std::uint8_t i = 0; i < _BYTES_PER_CONVERSION_PERIOD; ++i) {
        raw[i] = this->_readByte();
    }

    /**
     *  The HX711 requires a certain number of "positive clock
     *  pulses" depending on the set gain value.
     *  
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 4
     * 
     *  The expression below calculates the number of pulses
     *  after having read the three bytes above. For example,
     *  a gain of 128 requires 25 pulses: 24 pulses were made
     *  when reading the three bytes (3 * 8), so only one
     *  additional pulse is needed.
     */
    const std::uint8_t pulsesNeeded = 
        PULSES[static_cast<std::int32_t>(this->_gain)] -
            _BITS_PER_BYTE * _BYTES_PER_CONVERSION_PERIOD;

    for(std::uint8_t i = 0; i < pulsesNeeded; ++i) {
        this->_readBit();
    }

    lock.unlock();

    //if no byte pointer is given, don't try to write to it
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
     *  If this->_byteFormat indicates the HX711 is outputting
     *  bytes in LSB format, just reverse the array.
     * 
     *  Remember, the bytes param expects an array of bytes
     *  which will be converted to an int.
     */
    if(this->_byteFormat == Format::LSB) {
        std::reverse(raw, raw + _BYTES_PER_CONVERSION_PERIOD);
    }

    //finally, copy the local raw bytes to the byte array
    ::memcpy(bytes, raw, _BYTES_PER_CONVERSION_PERIOD);

}

std::int32_t HX711::_readInt() noexcept {

    std::uint8_t bytes[_BYTES_PER_CONVERSION_PERIOD];
    
    this->_readRawBytes(bytes);

    /**
     *  An int (int32_t) is 32 bits (4 bytes), but
     *  the HX711 only uses 24 bits (3 bytes).
     */
    const std::int32_t twosComp = ((       0 << 24) |
                                   (bytes[0] << 16) |
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
            _offset(0),
            _byteFormat(Format::MSB),
            _bitFormat(Format::MSB) {

                ::pinMode(this->_dataPin, INPUT);
                ::pinMode(this->_clockPin, OUTPUT);

                /**
                 *  Cannot simply set this->_gain. this->set_gain()
                 *  must be called to set the HX711 module at the
                 *  hardware-level.
                 */
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
    return ::digitalRead(this->_dataPin) == LOW;
}

void HX711::set_gain(const Gain gain) noexcept {
    this->_gain = gain;
    ::digitalWrite(this->_clockPin, LOW);
    this->_readRawBytes();
}

Gain HX711::get_gain() const noexcept {
    return this->_gain;
}

double HX711::get_value(const std::uint16_t times) noexcept {
    return this->get_value_A(times);
}

double HX711::get_value_A(const std::uint16_t times) noexcept {
    return this->readMedianValue(times) - this->getOffsetA();
}

double HX711::get_value_B(const std::uint16_t times) noexcept {
    const Gain gain = this->_gain;
    this->_gain = Gain::GAIN_32;
    const double val = this->readMedianValue(times) - this->getOffsetB();
    this->set_gain(gain);
    return val;
}

double HX711::get_weight(const std::uint16_t times) noexcept {
    return this->get_weight_A(times);
}

std::vector<double> HX711::get_weights(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }

    const double refUnit = static_cast<double>(this->_referenceUnit);

    std::vector<std::int32_t> rawValues = this->readValues(times);

    std::vector<double> values;
    values.reserve(times);

    for(std::uint16_t i = 0; i < times; ++i) {
        values.push_back((rawValues[i] / refUnit) - this->_offset);
    }

    return values;

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

    const double val = this->readMedianValue(times);

    this->setOffsetA(val);
    this->set_reference_unit_A(backupRefUnit);

    return val;

}

double HX711::tare_B(const std::uint16_t times) noexcept {

    const std::int32_t backupRefUnit = this->get_reference_unit_B();
    this->set_reference_unit_B(1);

    const Gain backupGain = this->_gain;
    this->set_gain(Gain::GAIN_32);

    const double val = this->readMedianValue(times);

    this->setOffsetB(val);
    this->set_gain(backupGain);
    this->set_reference_unit_B(backupRefUnit);

    return val;

}

void HX711::set_reading_format(const Format bitFormat, const Format byteFormat) noexcept {
    this->_bitFormat = bitFormat;
    this->_byteFormat = byteFormat;
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

std::vector<std::int32_t> HX711::readValues(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }

    std::vector<std::int32_t> values;
    values.reserve(times);

    for(std::uint16_t i = 0; i < times; ++i) {
        values.push_back(this->_readInt());
    }

    return values;

}

double HX711::readAverageValue(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    
    if(times == 1) {
        return static_cast<double>(this->_readInt());
    }

    std::vector<std::int32_t> values = this->readValues(times);

    const std::int64_t sum = std::accumulate(
        values.begin(), values.end(), 0);

    return static_cast<double>(sum) / values.size();

}

double HX711::readMedianValue(const std::uint16_t times) {

    if(times == 0) {
        throw std::invalid_argument("times must be greater than 0");
    }
    
    if(times == 1) {
        return (double)this->_readInt();
    }

    std::vector<std::int32_t> values = this->readValues(times);
    
    //https://stackoverflow.com/a/42791986/570787
    if(values.size() % 2 == 0) {

        const auto median_it1 = values.begin() + values.size() / 2 - 1;
        const auto median_it2 = values.begin() + values.size() / 2;

        std::nth_element(values.begin(), median_it1, values.end());
        const auto e1 = *median_it1;

        std::nth_element(values.begin(), median_it2, values.end());
        const auto e2 = *median_it2;

        return (e1 + e2) / 2.0;

    }
    else {
        const auto median_it = values.begin() + values.size() / 2;
        std::nth_element(values.begin(), median_it, values.end());
        return static_cast<double>(*median_it);
    }

}

void HX711::power_down() noexcept {

    std::lock_guard<std::mutex> lock(this->_readLock);

    ::digitalWrite(this->_clockPin, LOW);
    ::digitalWrite(this->_clockPin, HIGH);

    /**
     *  "When PD_SCK pin changes from low to high
     *  and stays at high for longer than 60µs, HX711
     *  enters power down mode (Fig.3)."
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5
     */
    ::delayMicroseconds(60);

}

void HX711::power_up() noexcept {

    std::unique_lock<std::mutex> lock(this->_readLock);

    ::digitalWrite(this->_clockPin, LOW);

    /**
     *  "When PD_SCK returns to low,
     *  chip will reset and enter normal operation mode"
     * 
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5   
     */

    lock.unlock();

    /**
     *  "After a reset or power-down event, input
     *  selection is default to Channel A with a gain of
     *  128."
     *  
     *  https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
     *  pg. 5 
     */
    if(this->_gain != Gain::GAIN_128) {
        this->_readRawBytes();
    }

}

void HX711::reset() noexcept {
    this->power_down();
    this->power_up();
}

};