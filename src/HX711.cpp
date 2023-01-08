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
#include <unordered_map>
#include <utility>
#include "../include/GpioException.h"
#include "../include/HX711.h"
#include "../include/IntegrityException.h"
#include "../include/TimeoutException.h"
#include "../include/Utility.h"

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
const std::unordered_map<const Gain, const unsigned int> HX711::_PULSES({
    { Gain::GAIN_128,       25 },
    { Gain::GAIN_32,        26 },
    { Gain::GAIN_64,        27 }
});

/**
 * Used to select the correct settling time depending on rate
 * Datasheet pg. 3
 */
const std::unordered_map<const Rate, const std::chrono::milliseconds> 
    HX711::_SETTLING_TIMES({
        { Rate::HZ_10, std::chrono::milliseconds(400) },
        { Rate::HZ_80, std::chrono::milliseconds(50) }
});

void HX711::_pulseClockNoRead() {

    //first, clock pin is set high to make DOUT ready to be read from
    //and the current ACTUAL time is noted for later
    const auto startNanos = Utility::getnanos();
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::HIGH);

    //then delay for sufficient time to allow DOUT to be ready (0.1us)
    //and min amount of time between the high to low clock pulse. Note
    //the overlap between T2 and T3
    if(this->_useDelays) {
        Utility::delay(std::max(_T2, _T3));
    }

    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);
    const auto diff = Utility::getnanos() - startNanos;

    //at this point, according to the documentation, if the clock pin
    //was held high for longer than 60us, the chip will have entered
    //power down mode. This means the currently read bit, and
    //consequently the entire value, is unreliable.
    if(this->_strictTiming && diff >= _POWER_DOWN_TIMEOUT) {
        throw IntegrityException("bit integrity failure");
    }

}

bool HX711::_readBit() const {

    const auto startNanos = Utility::getnanos();
    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::HIGH);

    if(this->_useDelays) {
        Utility::delay(std::max(_T2, _T3));
    }

    //at this stage, DOUT is ready so read the bit value
    const auto bit = Utility::readGpio(this->_gpioHandle, this->_dataPin);

    Utility::writeGpio(this->_gpioHandle, this->_clockPin, GpioLevel::LOW);
    const auto diff = Utility::getnanos() - startNanos;

    if(this->_strictTiming && diff >= _POWER_DOWN_TIMEOUT) {
        throw IntegrityException("bit integrity failure");
    }

    //Assuming everything was OK, the datasheet requires a further
    //delay before the next pulse
    if(this->_useDelays) {
        Utility::delay(_T4);
    }

    return static_cast<bool>(bit);

}

std::uint32_t HX711::_readBits() {

    std::lock_guard<std::mutex> lock(this->_commLock);

    if(this->_useDelays) {
        Utility::delay(_T1);
    }

    std::uint32_t v = 0;

    //msb first
    for(auto i = decltype(_BITS_PER_CONVERSION_PERIOD){0};
        i < _BITS_PER_CONVERSION_PERIOD;
        ++i) {
            *v <<= 1;
            *v |= this->_readBit();
    }

    this->_setInputGainSelection();

    return v;

}

bool HX711::_isReady() const {

    /**
     * HX711 will be "ready" when DOUT is low.
     * "Ready" means "data is ready for retrieval".
     * Datasheet pg. 4
     * 
     * This should be a one-shot test. Any follow-ups
     * or looping for checking if the sensor is ready
     * over time can/should be done by other calling code
     */
    return Utility::readGpio(
        this->_gpioHandle,
        this->_dataPin) == GpioLevel::LOW;

}

bool HX711::_waitReady(const std::chrono::microseconds timeout) const {

    using namespace std::chrono;

    const auto maxEnd = steady_clock::now();

    while(true) {

        if(this->_isReady()) {
            return true;
        }

        if(steady_clock::now() >= maxEnd) {
            return false;
        }

    }

}

HX711::HX711(
    const int dataPin,
    const int clockPin,
    const Rate rate) noexcept :
        _gpioHandle(-1),
        _dataPin(dataPin),
        _clockPin(clockPin),
        _rate(rate),
        _gain(Gain::GAIN_128),
        _strictTiming(false),
        _useDelays(false) {
}

void HX711::init() {

    if(this->_gpioHandle >= 0) {
        return;
    }

    this->_gpioHandle = Utility::openGpioHandle(0);
    Utility::openGpioInput(this->_gpioHandle, this->_dataPin);
    Utility::openGpioOutput(this->_gpioHandle, this->_clockPin);

}

void HX711::close() {

    if(this->_gpioHandle < 0) {
        return;
    }

    Utility::closeGpioPin(this->_gpioHandle, this->_clockPin);
    Utility::closeGpioPin(this->_gpioHandle, this->_dataPin);
    Utility::closeGpioHandle(this->_gpioHandle);

    this->_gpioHandle = -1;

}

void HX711::setGain(const Gain g) {
    this->_gain = g;
    this->getValue();
}

std::int32_t HX711::getValue() {
    while(!this->_isReady());
    return _convertFromTwosComplement(this->_readBits());
}

std::int32_t HX711::getValueTimeout(const std::chrono::microseconds timeout) {

    if(this->_waitReady(timeout)) {
        return _convertFromTwosComplement(this->_readBits());
    }
    else {
        throw TimeoutException();
    }
    
}

};