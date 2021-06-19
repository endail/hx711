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

#include "../include/HX711.h"
#include "../include/TimeoutException.h"
#include <chrono>
#include <lgpio.h>
#include <sys/time.h>

namespace HX711 {

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    return -(val & 0x800000) + (val & 0x7fffff);
}

bool HX711::_readBit() const noexcept {

    /**
     * The following code does not appear to work when using
     * std::this_thread::sleep_for. Code using delayMicroseconds
     * does work and is implemented below. I have left it here
     * for reference and how it should operate.
     * 
     * //first, clock pin is set high to make DOUT ready to be read from
     * ::digitalWrite(this->_clockPin, HIGH);
     * 
     * //!!!IMPORTANT NOTE!!!
     * //
     * //There is an overlap in timing between the clock pin changing from
     * //high to low and the data pin being ready to output the respective
     * //bit. This overlap is T2 in Fig.2 on pg. 5 of the datasheet.
     * //
     * //For the data pin to be ready, 0.1us needs to have elapsed.
     * std::this_thread::sleep_for(std::chrono::nanoseconds(100));
     * 
     * //at this stage, DOUT is ready to be read from
     * const bool bit = ::digitalRead(this->_dataPin) == HIGH;
     * 
     * //clock pin needs to be remain high for at least another 0.1us
     * std::this_thread::sleep_for(std::chrono::nanoseconds(100));
     * ::digitalWrite(this->_clockPin, LOW);

     * //once low, clock pin needs to remain low for at least 0.2us
     * //before the next bit can be read
     * //technically this doesn't need to exist here, but it is
     * //convenient
     * std::this_thread::sleep_for(std::chrono::nanoseconds(200));
     * 
     * return bit;
     */

    //first, clock pin is set high to make DOUT ready to be read from
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 1);

    //then delay for sufficient time to allow DOUT to be ready (0.1us)
    //this will also permit a sufficient amount of time for the clock
    //pin to remain high
    _delayMicroseconds(1);
    
    //at this stage, DOUT is ready and the clock pin has been held
    //high for sufficient amount of time, so read the bit value
    const bool bit = ::lgGpioRead(this->_gpioHandle, this->_dataPin) == 1;

    //the clock pin then needs to be held for at least 0.2us before
    //the next bit can be read
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 0);
    _delayMicroseconds(1);

    return bit;

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

    /**
     * Bytes are ready to be read from the HX711 when DOUT goes low. Therefore,
     * wait until this occurs.
     * Datasheet pg. 5
     * 
     * The - potential- issue is that DOUT going low does not appear to be
     * defined. It appears to occur whenever it is ready, whenever that is.
     * 
     * The code below should limit that to a reasonable time-frame of checking
     * after a predefined interval for a predefined number of attempts.
     */

    std::uint8_t tries = 0;

    do {

        if(this->isReady()) {
            break;
        }

        if(++tries < _MAX_READ_TRIES) {
            //::lguSleep(_WAIT_INTERVAL_US / static_cast<double>(std::chrono::microseconds::period::den));
            _delayMicroseconds(_WAIT_INTERVAL_US);
        }
        else {
            throw TimeoutException("timed out while trying to read bytes from HX711");
        }

    }
    while(true);

    //this must occur AFTER the isReady call otherwise it
    //will deadlock
    std::unique_lock<std::mutex> lock(this->_readLock);

    /**
     * When DOUT goes low, there is a minimum of 0.1us until the clock pin
     * can go high. T1 in Fig.2.
     * Datasheet pg. 5
     */
    _delayMicroseconds(1);
    

    //delcare array of bytes of sufficient size
    //uninitialised is fine; they'll be overwritten
    std::uint8_t raw[_BYTES_PER_CONVERSION_PERIOD];

    //then populate it with values from the hx711
    for(std::uint8_t i = 0; i < _BYTES_PER_CONVERSION_PERIOD; ++i) {
        raw[i] = this->_readByte();
    }

    /**
     * The HX711 requires a certain number of "positive clock
     * pulses" depending on the set gain value.
     * Datasheet pg. 4
     * 
     * The expression below calculates the number of pulses
     * after having read the three bytes above. For example,
     * a gain of 128 requires 25 pulses: 24 pulses were made
     * when reading the three bytes (3 * 8), so only one
     * additional pulse is needed.
     */
    const std::uint8_t pulsesNeeded = 
        PULSES[static_cast<std::size_t>(this->_gain)] -
            8 * _BYTES_PER_CONVERSION_PERIOD;

    for(std::uint8_t i = 0; i < pulsesNeeded; ++i) {
        this->_readBit();
    }

    //not reading from the sensor any more so no need to keep
    //the lock in place
    lock.unlock();

    //if no byte pointer is given, don't try to write to it
    if(bytes == nullptr) {
        return;
    }

    /**
     * The HX711 will supply bits in big-endian format;
     * the 0th read bit is the MSB.
     * Datasheet pg. 4
     * 
     * If this->_byteFormat indicates the HX711 is outputting
     * bytes in LSB format, swap the first and last bytes
     * 
     * Remember, the bytes param expects an array of bytes
     * which will be converted to an int.
     */
    if(this->_byteFormat == Format::LSB) {
        const std::uint8_t swap = raw[0];
        raw[0] = raw[_BYTES_PER_CONVERSION_PERIOD - 1];
        raw[_BYTES_PER_CONVERSION_PERIOD - 1] = swap;
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
     * An int (int32_t) is 32 bits (4 bytes), but
     * the HX711 only uses 24 bits (3 bytes).
     */
    const std::int32_t twosComp = ((       0 << 24) |
                                   (bytes[0] << 16) |
                                   (bytes[1] << 8)  |
                                    bytes[2]         );

    return _convertFromTwosComplement(twosComp);

}

void HX711::_delayMicroseconds(const unsigned int us) noexcept {

    //https://github.com/WiringPi/WiringPi/blob/master/wiringPi/wiringPi.c#L2144

    struct timeval tNow;
    struct timeval tLong;
    struct timeval tEnd;

    tLong.tv_sec = us / 1000000;
    tLong.tv_usec = us % 1000000;

    ::gettimeofday(&tNow, nullptr);
    timeradd(&tNow, &tLong, &tEnd);

    while(timercmp(&tNow, &tEnd, <)) {
        ::gettimeofday(&tNow, nullptr);
    }

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
}

HX711::~HX711() {
    ::lgGpioFree(this->_gpioHandle, this->_clockPin);
    ::lgGpioFree(this->_gpioHandle, this->_dataPin);
    ::lgGpiochipClose(this->_gpioHandle);
}

void HX711::begin() {

    if(!(   (this->_gpioHandle = ::lgGpiochipOpen(0)) >= 0 &&
            ::lgGpioClaimInput(this->_gpioHandle, 0, this->_dataPin) == 0 &&
            ::lgGpioClaimOutput(this->_gpioHandle, 0, this->_clockPin, 0) >= 0
    )) {
        throw std::runtime_error("unable to access GPIO");
    }

    /**
     * Cannot simply set this->_gain. this->setGain()
     * must be called to set the HX711 module at the
     * hardware-level.
     * 
     * If, for whatever reason, the sensor cannot be
     * reached, setGain will fail and throw a
     * TimeoutException. Calling code can catch this
     * and handle as though the sensor connection has
     * "failed".
     * 
     * try {
     *     hx.begin();
     * }
     * catch(TimeoutException& e) {
     *     //sensor failed to connect
     * }
     */
    this->setGain(this->_gain);

}

bool HX711::isReady() noexcept {

    /**
     * The datasheet states DOUT is used to shift-out data,
     * and in the process DOUT will either be HIGH or LOW
     * to represent bits of the resulting integer. The issue
     * is that during the "conversion period" of shifting
     * bits out, DOUT could be LOW, but not necessarily
     * mean there is "new" data for retrieval. Page 4 states
     * that the "25th pulse at PD_SCK input will pull DOUT
     * pin back to high".
     * 
     * This is justification enough to guard against
     * potentially erroneous "ready" states while a
     * conversion is in progress. The lock is already in
     * place to prevent extra reads from the sensor, so
     * it should suffice to stop this issue as well.
     */
    std::lock_guard<std::mutex> lock(this->_readLock);

    /**
     * HX711 will be "ready" when DOUT is low.
     * "Ready" means "data is ready for retrieval".
     * Datasheet pg. 4
     * 
     * This should be a one-shot test. Any follow-ups
     * or looping for checking if the sensor is ready
     * over time can/should be done by other calling code
     */
    return ::lgGpioRead(this->_gpioHandle, this->_dataPin) == 0;

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
        
        /**
         * A read must take place to set the gain at the
         * hardware level. See datasheet pg. 4 "Serial
         * Interface".
         */
        this->_readRawBytes();
        
    }
    catch(const TimeoutException& e) {
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

    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 0);
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 1);

    /**
     * "When PD_SCK pin changes from low to high
     * and stays at high for longer than 60µs, HX711
     * enters power down mode (Fig.3)."
     * Datasheet pg. 5
     */
    _delayMicroseconds(60);

}

void HX711::powerUp() {

    std::unique_lock<std::mutex> lock(this->_readLock);

    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 0);

    /**
     * "When PD_SCK returns to low,
     * chip will reset and enter normal operation mode"
     * Datasheet pg. 5
     */

    lock.unlock();

    /**
     * "After a reset or power-down event, input
     * selection is default to Channel A with a gain of
     * 128."
     * Datasheet pg. 5
     * 
     * This means the following statement to set the gain
     * is needed ONLY IF the current gain isn't 128
     */
    if(this->_gain != Gain::GAIN_128) {
        this->setGain(this->_gain);
    }

}

};