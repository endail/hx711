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

#include <chrono>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <time.h>
#include <unordered_map>
#include <utility>
#include "../include/GpioException.h"
#include "../include/HX711.h"
#include "../include/IntegrityException.h"
#include "../include/TimeoutException.h"
#include "../include/Utility.h"
#include "../include/Value.h"

namespace HX711 {

constexpr std::chrono::nanoseconds HX711::_T1;
constexpr std::chrono::nanoseconds HX711::_T2;
constexpr std::chrono::nanoseconds HX711::_T3;
constexpr std::chrono::nanoseconds HX711::_T4;
constexpr std::chrono::microseconds HX711::_POWER_DOWN_TIMEOUT;

/**
 * Used to select the correct number of clock pulses depending on the
 * gain
 * Datasheet pg. 4
 */
const std::unordered_map<const Gain, const unsigned char> HX711::_PULSES({
    { Gain::GAIN_128,       25 },
    { Gain::GAIN_32,        26 },
    { Gain::GAIN_64,        27 }
});

/**
 * Used to select the correct settling time depending on rate
 * Datasheet pg. 3
 */
const std::unordered_map<const Rate, const std::chrono::nanoseconds> 
    HX711::_SETTLING_TIMES({
        { Rate::HZ_10, std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(400)) },
        { Rate::HZ_80, std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds(50)) }
});

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    return -(val & 0x800000) + (val & 0x7fffff);
}

unsigned char HX711::_calculatePulses(const Gain g) noexcept {
    return _PULSES.at(g) - _BITS_PER_CONVERSION_PERIOD;
}

void HX711::_setInputGainSelection() {

    const unsigned char pulses = _calculatePulses(this->_gain);

    for(unsigned char i = 0; i < pulses; ++i) {
        this->_readBit();
    }

}

bool HX711::isReady() const {

    /**
     * HX711 will be "ready" when DOUT is low.
     * "Ready" means "data is ready for retrieval".
     * Datasheet pg. 4
     * 
     * This should be a one-shot test. Any follow-ups
     * or looping for checking if the sensor is ready
     * over time can/should be done by other calling code
     */
    try {
        return Utility::readGpio(
            this->_gpioHandle,
            this->_dataPin) == GpioLevel::LOW;
    }
    catch(const GpioException& ex) {
        return false;
    }

}

bool HX711::_readBit() const {

    //TODO: do these need to be 0-init'd?
    ::timespec startTime;
    ::timespec endTime;

    //first, clock pin is set high to make DOUT ready to be read from
    //and the current ACTUAL time is noted for later
    ::clock_gettime(CLOCK_REALTIME, &startTime);
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::HIGH);

    //then delay for sufficient time to allow DOUT to be ready (0.1us)
    //this will also permit a sufficient amount of time for the clock
    //pin to remain high
    //
    //In practice this [probably] isn't really going to matter
    //due to how miniscule the amount of time is and how slow the
    //execution of the code is in comparison. For that reason, the
    //delay below is excluded
    //Utility::delayns(std::max(_T2, _T3));

    //at this stage, DOUT is ready and the clock pin has been held
    //high for sufficient amount of time, so read the bit value
    const bool bit = static_cast<bool>(
        Utility::readGpio(this->_gpioHandle, this->_dataPin));

    //with the bit value now read, set the clock pin low and note the
    //current ACTUAL time again
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);
    ::clock_gettime(CLOCK_REALTIME, &endTime);

    //At this point, according to the documentation, if the clock pin
    //was held high for longer than 60us, the chip will have entered
    //power down mode. This means the currently read bit is unreliable.
    //
    //So... first, calculate the difference
    const std::chrono::nanoseconds diff = Utility::timespec_to_nanos(&endTime) -
        Utility::timespec_to_nanos(&startTime);

    //...and if the threshold was exceeded, raise the exception
    if(this->_strictTiming && diff >= _POWER_DOWN_TIMEOUT) {
        throw IntegrityException("bit integrity failure");
    }

    //Assuming everything was OK, the datasheet requires a further
    //delay. But, as for the reasons previously mentioned above, the
    //following delay is probably not going to matter and is therefore
    //excluded.
    //Utility::delayns(_T4);

    return bit;

}

void HX711::_readBits(std::int32_t* const v) {

    std::lock_guard<std::mutex> lock(this->_commLock);

    //The datasheet notes a tiny delay between DOUT going low and the
    //initial clock pin change. But, as for the reasons previously
    //mentioned above, the following delay is probably not going to
    //matter and is therefore excluded.
    //Utility::delayns(_T1);

    //msb first
    for(unsigned char i = 0; i < _BITS_PER_CONVERSION_PERIOD; ++i) {
        *v <<= 1;
        *v |= this->_readBit();
    }

    this->_setInputGainSelection();

}

HX711::HX711(const int dataPin, const int clockPin, const Rate rate) noexcept :
    _gpioHandle(-1),
    _dataPin(dataPin),
    _clockPin(clockPin),
    _rate(rate),
    _channel(Channel::A),
    _gain(Gain::GAIN_128),
    _strictTiming(false),
    _bitFormat(Format::MSB) {
}

HX711::~HX711() {
    Utility::closeGpioPin(this->_gpioHandle, this->_clockPin);
    Utility::closeGpioPin(this->_gpioHandle, this->_dataPin);
    Utility::closeGpioHandle(this->_gpioHandle);
}

void HX711::begin() {

    this->_gpioHandle = Utility::openGpioHandle(0);
    Utility::openGpioInput(this->_gpioHandle, this->_dataPin);
    Utility::openGpioOutput(this->_gpioHandle, this->_clockPin);

    this->setConfig(this->_channel, this->_gain);

}

void HX711::setStrictTiming(const bool strict) noexcept {
    std::lock_guard<std::mutex> lock(this->_commLock);
    this->_strictTiming = strict;
}

void HX711::setFormat(const Format bitFormat) noexcept {
    std::lock_guard<std::mutex> lock(this->_commLock);
    this->_bitFormat = bitFormat;
}

int HX711::getDataPin() const noexcept {
    return this->_dataPin;
}

int HX711::getClockPin() const noexcept {
    return this->_clockPin;
}

Channel HX711::getChannel() const noexcept {
    return this->_channel;
}

Gain HX711::getGain() const noexcept {
    return this->_gain;
}

void HX711::setConfig(const Channel c, const Gain g) {

    if(c == Channel::A && g == Gain::GAIN_32) {
        throw std::invalid_argument("Channel A can only use a gain of 128 or 64");
    }
    else if(c == Channel::B && g != Gain::GAIN_32) {
        throw std::invalid_argument("Channel B can only use a gain of 32");
    }

    const auto backupChannel = this->_channel;
    const auto backupGain = this->_gain;

    this->_channel = c;
    this->_gain = g;

    /**
     * If the attempt to set the gain fails, it should
     * revert back to whatever it was before
     */
    try {

        /**
         * A read must take place to set the gain at the
         * hardware level. See datasheet pg. 4 "Serial
         * Interface".
         */
        while(!this->isReady());
        this->readValue();

        /**
         * If PD_SCK pulse number is changed during
         * the current conversion period, power down should
         * be executed after current conversion period is
         * completed. This is to ensure that the change is
         * saved. When chip returns back to normal
         * operation from power down, it will return to the
         * set up conditions of the last change.
         * 
         * Datasheet pg. 5
         */
        this->powerDown();
        this->powerUp();

    }
    catch(const TimeoutException& e) {
        this->_channel = backupChannel;
        this->_gain = backupGain;
        throw;
    }

}

Value HX711::readValue() {

    std::int32_t v = 0;

    this->_readBits(&v);

    if(this->_bitFormat == Format::LSB) {
        v = Utility::reverse(v);
    }

    return Value(_convertFromTwosComplement(v));
    
}

void HX711::powerDown() {

    std::lock_guard<std::mutex> lock(this->_commLock);

    /**
     * The delay between low to high is probably not necessary, but it
     * should help to keep the underlying code from optimising it away -
     * if does at all.
     */
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);
    Utility::delayus(std::chrono::microseconds(1));
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::HIGH);

    /**
     * "When PD_SCK pin changes from low to high
     * and stays at high for longer than 60µs, HX711
     * enters power down mode (Fig.3)."
     * Datasheet pg. 5
     */
    Utility::sleepns(_POWER_DOWN_TIMEOUT);

}

void HX711::powerUp() {

    std::lock_guard<std::mutex> lock(this->_commLock);

    /**
     * "When PD_SCK returns to low,
     * chip will reset and enter normal operation mode"
     * Datasheet pg. 5
     */
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);

    if(this->_rate != Rate::OTHER) {
        Utility::sleepns(_SETTLING_TIMES.at(this->_rate));
    }

}

};