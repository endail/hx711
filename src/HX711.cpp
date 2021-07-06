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
#include <algorithm>
#include <iterator>
#include <utility>
#include <stdexcept>
#include <thread>
#include <lgpio.h>
#include <sys/time.h>

namespace HX711 {

constexpr std::chrono::nanoseconds HX711::_DEFAULT_MAX_WAIT;

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    return -(val & 0x800000) + (val & 0x7fffff);
}

std::uint8_t HX711::_calculatePulses(const Gain g) noexcept {
    return PULSES[static_cast<std::uint8_t>(g)] -
        8 * _BYTES_PER_CONVERSION_PERIOD;
}

bool HX711::_isReady() noexcept {

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

bool HX711::_readBit() noexcept {

    using namespace std::chrono;

    //first, clock pin is set high to make DOUT ready to be read from
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 1);

    //then delay for sufficient time to allow DOUT to be ready (0.1us)
    //this will also permit a sufficient amount of time for the clock
    //pin to remain high
    _delayns(duration_cast<nanoseconds>(microseconds(1)));

    //at this stage, DOUT is ready and the clock pin has been held
    //high for sufficient amount of time, so read the bit value
    const bool bit = ::lgGpioRead(this->_gpioHandle, this->_dataPin) == 1;

    //the clock pin then needs to be held for at least 0.2us before
    //the next bit can be read
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 0);
    _delayns(duration_cast<nanoseconds>(microseconds(1)));

    return bit;

}

BYTE HX711::_readByte() noexcept {

    BYTE val = 0;

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

void HX711::_readRawBytes(BYTE* bytes) {

    using namespace std::chrono;

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

    std::unique_lock<std::mutex> communication(this->_commLock);

    /**
     * When DOUT goes low, there is a minimum of 0.1us until the clock pin
     * can go high. T1 in Fig.2.
     * Datasheet pg. 5
     */
    _delayns(duration_cast<nanoseconds>(microseconds(1)));

    //delcare array of bytes of sufficient size
    //uninitialised is fine; they'll be overwritten
    BYTE raw[_BYTES_PER_CONVERSION_PERIOD];

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
    const std::uint8_t pulsesNeeded = _calculatePulses(this->_gain);

    for(std::uint8_t i = 0; i < pulsesNeeded; ++i) {
        this->_readBit();
    }

    //not reading from the sensor any more so no need to keep
    //the lock in place
    communication.unlock();

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
        std::swap(raw[0], raw[_BYTES_PER_CONVERSION_PERIOD - 1]);
    }

    //finally, copy the local raw bytes to the byte array
    std::copy(std::begin(raw), std::end(raw), std::begin(bytes));

}

Value HX711::_readInt() {

    BYTE bytes[_BYTES_PER_CONVERSION_PERIOD];
    
    this->_readRawBytes(bytes);

    /**
     * An int (int32_t) is 32 bits (4 bytes), but
     * the HX711 only uses 24 bits (3 bytes).
     */
    const std::int32_t twosComp = ((       0 << 24) |
                                   (bytes[0] << 16) |
                                   (bytes[1] << 8)  |
                                    bytes[2]         );

    return Value(_convertFromTwosComplement(twosComp));

}

void HX711::_sleepns(const std::chrono::nanoseconds ns) noexcept {
    //TODO: in this context, this_thread::sleep_for may be more appropiate
    ::lguSleep(static_cast<double>(ns.count()) / decltype(ns)::period::den);
    //std::this_thread::sleep_for(ns);
}

void HX711::_delayns(const std::chrono::nanoseconds ns) noexcept {

    /**
     * This requires some explanation.
     * 
     * Delays on a pi are inconsistent due to the OS not being a real-time OS.
     * A previous version of this code used wiringPi which used its
     * delayMicroseconds function to delay in the microsecond range. The way this
     * was implemented was with a busy-wait loop for times under 100 nanoseconds.
     * 
     * https://github.com/WiringPi/WiringPi/blob/f15240092312a54259a9f629f9cc241551f9faae/wiringPi/wiringPi.c#L2165-L2166
     * https://github.com/WiringPi/WiringPi/blob/f15240092312a54259a9f629f9cc241551f9faae/wiringPi/wiringPi.c#L2153-L2154
     * 
     * This (the busy-wait) would, presumably, help to prevent context switching
     * therefore keep the timing required by the HX711 module relatively
     * consistent.
     * 
     * When this code changed to using the lgpio library, its lguSleep function
     * appeared to be an equivalent replacement. But it did not work.
     * 
     * http://abyz.me.uk/lg/lgpio.html#lguSleep
     * https://github.com/joan2937/lg/blob/8f385c9b8487e608aeb4541266cc81d1d03514d3/lgUtil.c#L56-L67
     * 
     * The problem appears to be that lguSleep is not busy-waiting. And, when
     * a sleep occurs, it is taking far too long to return. Contrast this
     * behaviour with wiringPi, which constantly calls gettimeofday until return.
     * 
     * In short, use this function for delays under 100us.
     */

    using namespace std::chrono;

    struct timeval tNow;
    struct timeval tLong;
    struct timeval tEnd;

    const uint8_t us = duration_cast<microseconds>(ns).count();

    tLong.tv_sec = us / decltype(microseconds)::den;
    tLong.tv_usec = us % decltype(microseconds)::den;

    ::gettimeofday(&tNow, nullptr);
    timeradd(&tNow, &tLong, &tEnd);

    while(timercmp(&tNow, &tEnd, <)) {
        ::gettimeofday(&tNow, nullptr);
    }

}

void HX711::_watchPin() {
    
    /**
     * This is the thread loop function for watching when data is ready from
     * the HX711.
     * 
     * A state var is used to control what operation(s) this thread is
     * performing.
     *      NONE:   undefined state and should not be used
     *      NORMAL: normal operation of obtaining sensor values and sleeping
     *      PAUSE:  causes the thread to stop performing any operations without
     *              causing the thread to end
     *      END:    lets the thread exit - not recoverable
     * 
     * Two relevant locks are used: state and data ready
     * 
     * State Lock:
     * In order for this loop's state to be changed, a state var is used. But
     * the mechanism for changing state needs to be atomic - it must not be
     * altered while the current state is executing. Thefore, at the beginning
     * of each loop, the state is locked. This does not actually "lock" the var
     * but controls access to it with the _changeWatchState function. The state
     * lock should be unlocked prior to the next iteration of the loop, but
     * prior to [any] sleep/yield calls. This will give any other thread(s) the
     * best opportunity to acquire a lock and change the state while this
     * thread is asleep and/or yielding.
     * 
     * DataReady Lock:
     * This lock works in conjunction with a condition variable in order to
     * notify the caller when a new sensor reading is available for be used.
     * It should not be unlocked until a usable value has been obtained. A
     * caller can timeout while waiting if needed.
     */

    //TODO: can <atomic> be used to sync state?

    std::unique_lock<std::mutex> stateLock(this->_pinWatchLock, std::defer_lock);
    std::unique_lock<std::mutex> dataReadyLock(this->_readyLock, std::defer_lock);
    
    for(;;) {

        stateLock.lock();

        if(this->_watchState == PinWatchState::END) {
            break;
        }
        
        if(this->_watchState == PinWatchState::PAUSE) {
            stateLock.unlock();
            std::this_thread::yield();
            _sleepns(this->_pauseSleep);
            continue;
        }

        if(!dataReadyLock.owns_lock()) {
            dataReadyLock.lock();
        }

        if(!this->_isReady()) {
            stateLock.unlock();
            _sleepns(this->_notReadySleep);
            continue;
        }

        const Value v = this->_readInt();

        if(!v.isValid()) {
            stateLock.unlock();
            //sleep if out of range
            //TODO: implement member for this
            //_sleepns();
            continue;
        }

        this->_lastVal = v;
        this->_dataReady.notify_all();
        dataReadyLock.unlock();
        stateLock.unlock();
        _sleepns(this->_pollSleep);

    }

    /**
     * Any thread cleanup stuff goes here, under the loop
     */

}

void HX711::_changeWatchState(const PinWatchState state) {
    std::lock_guard<std::mutex> lck(this->_pinWatchLock);
    this->_watchState = state;
}

HX711::HX711(const int dataPin, const int clockPin) noexcept :
    _gpioHandle(-1),
    _dataPin(dataPin),
    _clockPin(clockPin),
    _lastVal(Value::MIN),
    _watchState(PinWatchState::PAUSE),
    _pauseSleep(_DEFAULT_PAUSE_SLEEP),
    _notReadySleep(_DEFAULT_NOT_READY_SLEEP),
    _saturatedSleep(_DEFAULT_SATURATED_SLEEP),
    _pollSleep(_DEFAULT_POLL_SLEEP),
    _rate(Rate::HZ_10),
    _channel(Channel::A),
    _gain(Gain::GAIN_128),
    _bitFormat(Format::MSB),
    _byteFormat(Format::MSB) {
}

HX711::~HX711() {
    this->_changeWatchState(PinWatchState::END);
    ::lgGpioFree(this->_gpioHandle, this->_clockPin);
    ::lgGpioFree(this->_gpioHandle, this->_dataPin);
    ::lgGpiochipClose(this->_gpioHandle);
}

void HX711::begin() {

    //TODO: move this to constructor?

    if(!(   
        (this->_gpioHandle = ::lgGpiochipOpen(0)) >= 0 &&
        ::lgGpioClaimInput(this->_gpioHandle, 0, this->_dataPin) == 0 &&
        ::lgGpioClaimOutput(this->_gpioHandle, 0, this->_clockPin, 0) == 0
    )) {
        throw std::runtime_error("unable to access GPIO");
    }

    std::thread(&HX711::_watchPin, this).detach();

    this->powerDown();
    this->powerUp();

}

std::vector<Timing> HX711::testTiming(const std::size_t samples) noexcept {

    using namespace std::chrono;

    std::vector<Timing> vec;
    vec.reserve(samples);

    for(size_t i = 0; i < samples; ++i) {

        Timing t;

        t.begin = high_resolution_clock::now();
        
        while(!this->_isReady());
        t.ready = high_resolution_clock::now();

        this->_readInt();
        t.end = high_resolution_clock::now();

        while(!this->_isReady()) ;
        t.nextbegin = high_resolution_clock::now();

        vec.push_back(t);

    }

    return vec;

}

Value HX711::getValue() {
    Value v;
    this->getValues(&v, 1);
    return v;
}

void HX711::getValues(
    Value* const arr,
    const std::size_t len,
    const std::chrono::nanoseconds maxWait) {

        std::unique_lock<std::mutex> ready(this->_readyLock, std::defer_lock);

        this->_changeWatchState(PinWatchState::NORMAL);

        for(size_t i = 0; i < len; ++i) {
            
            ready.lock();

            if(this->_dataReady.wait_for(ready, maxWait) == std::cv_status::timeout) {
                throw TimeoutException("timed out while trying to read from HX711");
            }

            arr[i] = this->_lastVal;
            ready.unlock();

        }

        this->_changeWatchState(PinWatchState::PAUSE);

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

    const Channel backupChannel = this->_channel;
    const Gain backupGain = this->_gain;

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
        this->getValue();
        
    }
    catch(const TimeoutException& e) {
        this->_channel = backupChannel;
        this->_gain = backupGain;
        throw;
    }

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

    using namespace std::chrono;

    std::lock_guard<std::mutex> lock(this->_commLock);

    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 0);
    _delayns(duration_cast<nanoseconds>(microseconds(1)));
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 1);

    /**
     * "When PD_SCK pin changes from low to high
     * and stays at high for longer than 60µs, HX711
     * enters power down mode (Fig.3)."
     * Datasheet pg. 5
     */
    _sleepns(duration_cast<nanoseconds>(microseconds(60)));

}

void HX711::powerUp() {

    std::unique_lock<std::mutex> lock(this->_commLock);

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
        this->setConfig(this->_channel, this->_gain);
    }

}

};