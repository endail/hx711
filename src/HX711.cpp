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
#include "../include/TimeoutException.h"
#include <thread>
#include <wiringPi.h>

namespace HX711 {

constexpr std::chrono::microseconds HX711::_WAIT_INTERVAL;

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    const std::int32_t maskA = 0x800000;
    const std::int32_t maskB = 0x7fffff;
    return -(val & maskA) + (val & maskB);
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
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    ::digitalWrite(this->_clockPin, LOW);
    std::this_thread::sleep_for(std::chrono::microseconds(1));

    return ::digitalRead(this->_dataPin) == HIGH;

}

std::uint8_t HX711::_readByte() const noexcept {

    std::uint8_t val = 0;

    //8 bits per byte...
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

void HX711::_readRawBytes(std::uint8_t* bytes) {

    std::unique_lock<std::mutex> lock(this->_readLock);

    /**
     *  Bytes are ready to be read from the HX711 when DOUT goes low. Therefore,
     *  wait until this occurs.
     *  Datasheet pg. 5
     */

    std::uint8_t tries = 0;

    do {

        if(this->isReady()) {
            break;
        }

        if(++tries < _MAX_READ_TRIES) {
            std::this_thread::sleep_for(_WAIT_INTERVAL);
        }
        else {
            throw TimeoutException("timed out while trying to read bytes from HX711");
        }

    }
    while(true);

    /**
     *  When DOUT goes low, there is a minimum of 0.1us until the clock pin
     *  can go high. T1 in Fig.2.
     *  Datasheet pg. 5
     *  0.1us == 100ns
     */
    std::this_thread::sleep_for(std::chrono::nanoseconds(100));

    //delcare array of bytes of sufficient size
    std::uint8_t raw[_BYTES_PER_CONVERSION_PERIOD];

    //then populate it with values from the hx711
    for(std::uint8_t i = 0; i < _BYTES_PER_CONVERSION_PERIOD; ++i) {
        raw[i] = this->_readByte();
    }

    /**
     *  The HX711 requires a certain number of "positive clock
     *  pulses" depending on the set gain value.
     *  Datasheet pg. 4
     * 
     *  The expression below calculates the number of pulses
     *  after having read the three bytes above. For example,
     *  a gain of 128 requires 25 pulses: 24 pulses were made
     *  when reading the three bytes (3 * 8), so only one
     *  additional pulse is needed.
     */
    const std::uint8_t pulsesNeeded = 
        PULSES[static_cast<std::size_t>(this->_gain)] -
            8 * _BYTES_PER_CONVERSION_PERIOD;

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
     *  Datasheet pg. 4
     * 
     *  If this->_byteFormat indicates the HX711 is outputting
     *  bytes in LSB format, swap the first and last bytes
     * 
     *  Remember, the bytes param expects an array of bytes
     *  which will be converted to an int.
     */
    if(this->_byteFormat == Format::LSB) {
        const std::uint8_t swap = raw[0];
        raw[0] = raw[2];
        raw[2] = swap;
    }

    //finally, copy the local raw bytes to the byte array
    for(std::uint8_t i = 0; i < _BYTES_PER_CONVERSION_PERIOD; ++i) {
        bytes[i] = raw[i];
    }

}

HX_VALUE HX711::_readInt() {

    std::uint8_t bytes[_BYTES_PER_CONVERSION_PERIOD];
    
    this->_readRawBytes(bytes);

    /**
     *  An int (int32_t) is 32 bits (4 bytes), but
     *  the HX711 only uses 24 bits (3 bytes).
     */
    const std::int32_t twosComp = ((       0 << 24) |
                                   (bytes[0] << 16) |
                                   (bytes[1] << 8)  |
                                    bytes[2]         );

    return _convertFromTwosComplement(twosComp);

}

HX_VALUE HX711::_getChannelAValue() {

    /**
     * "Channel A can be programmed with a gain 
     * of 128 or 64..."
     * Datasheet pg. 1
     * 
     * Opt to default to 128
     */
    if(this->_gain == Gain::GAIN_32) {
        this->setGain(Gain::GAIN_128);
    }

    return this->_readInt();

}

HX_VALUE HX711::_getChannelBValue() {
    
    /**
     * "Channel B has a fixed gain of 32"
     * Datasheet pg. 1
     */
    if(this->_gain != Gain::GAIN_32) {
        this->setGain(Gain::GAIN_32);
    }

    return this->_readInt();

}

HX711::HX711(
    const int dataPin,
    const int clockPin) noexcept :
        _dataPin(dataPin),
        _clockPin(clockPin) {
            ::pinMode(this->_dataPin, INPUT);
            ::pinMode(this->_clockPin, OUTPUT);
}

void HX711::connect(
    const Gain gain,
    const Format bitFormat,
    const Format byteFormat) {

        this->setBitFormat(bitFormat);
        this->setByteFormat(byteFormat);

        /**
         *  Cannot simply set this->_gain. this->setGain()
         *  must be called to set the HX711 module at the
         *  hardware-level.
         * 
         *  If, for whatever reason, the sensor cannot be
         *  reached, setGain will fail and throw a
         *  TimeoutException. Calling code can catch this
         *  and handle as though the sensor connection has
         *  "failed".
         * 
         *  try {
         *      sensor.connect();
         *  }
         *  catch(TimeoutException& e) {
         *      //sensor failed to connect
         *  }
         */
        this->setGain(gain);

}

bool HX711::isReady() const noexcept {
    /**
     *  HX711 will be "ready" when DOUT is low.
     *  Datasheet pg. 5
     * 
     *  This should be a one-shot test. Any follow-ups
     *  or looping for checking if the sensor is ready
     *  over time can/should be done by other calling code
     */
    return ::digitalRead(this->_dataPin) == LOW;
}

HX_VALUE HX711::getValue(const Channel c) {

    if(c == Channel::A) {
        return this->_getChannelAValue();
    }
    
    //else channel B
    return this->_getChannelBValue();

}

int HX711::getDataPin() const noexcept {
    return this->_dataPin;
}

int HX711::getClockPin() const noexcept {
    return this->_clockPin;
}

void HX711::setGain(const Gain gain) {

    const Gain backup = this->_gain;

    /**
     * If the attempt to set the gain fails, it should
     * revert back to whatever it was before
     */
    try {

        this->_gain = gain;

        //why is this here?
        //remove if not necessary
        ::digitalWrite(this->_clockPin, LOW);
        
        /**
         * A read must take place to set the gain at the
         * hardware level. See datasheet pg. 4 "Serial
         * Interface".
         */
        this->_readRawBytes();
        
    }
    catch(TimeoutException& e) {
        this->_gain = backup;
        throw;
    }

}

Gain HX711::getGain() const noexcept {
    return this->_gain;
}

Format HX711::getBitFormat() const noexcept {
    return this->_bitFormat;
}

Format HX711::getByteFormat() const noexcept {
    return this->_byteFormat;
}

void HX711::setBitFormat(const Format f) noexcept {
    this->_bitFormat = f;
}

void HX711::setByteFormat(const Format f) noexcept {
    this->_byteFormat = f;
}

void HX711::powerDown() noexcept {

    std::lock_guard<std::mutex> lock(this->_readLock);

    ::digitalWrite(this->_clockPin, LOW);
    ::digitalWrite(this->_clockPin, HIGH);

    /**
     *  "When PD_SCK pin changes from low to high
     *  and stays at high for longer than 60µs, HX711
     *  enters power down mode (Fig.3)."
     *  Datasheet pg. 5
     */
    std::this_thread::sleep_for(std::chrono::microseconds(60));

}

void HX711::powerUp() {

    std::unique_lock<std::mutex> lock(this->_readLock);

    ::digitalWrite(this->_clockPin, LOW);

    /**
     *  "When PD_SCK returns to low,
     *  chip will reset and enter normal operation mode"
     *  Datasheet pg. 5
     */

    lock.unlock();

    /**
     *  "After a reset or power-down event, input
     *  selection is default to Channel A with a gain of
     *  128."
     *  Datasheet pg. 5
     * 
     *  This means the following statement to set the gain
     *  is needed ONLY IF the current gain isn't 128
     */
    if(this->_gain != Gain::GAIN_128) {
        this->setGain(this->_gain);
    }

}

};