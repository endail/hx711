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
#include <cassert>
#include <cerrno>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <lgpio.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

namespace HX711 {

Value::operator _INTERNAL_TYPE() const noexcept {
    return this->_v;
}

bool Value::isSaturated() const noexcept {
    return this->_v == _MIN || this->_v == _MAX;
}

bool Value::isValid() const noexcept {
    return this->_v >= _MIN && this->_v <= _MAX;
}

Value::Value(const _INTERNAL_TYPE v) noexcept : _v(v) {
}

Value::Value() noexcept : _v(std::numeric_limits<_INTERNAL_TYPE>::min()) {
}

Value& Value::operator=(const Value& v2) noexcept {
    this->_v = v2._v;
    return *this;
}

constexpr std::chrono::nanoseconds HX711::_DEFAULT_MAX_WAIT;

std::int32_t HX711::_convertFromTwosComplement(const std::int32_t val) noexcept {
    return -(val & 0x800000) + (val & 0x7fffff);
}

std::uint8_t HX711::_calculatePulses(const Gain g) noexcept {
    return _PULSES.at(g) - (8 * _BYTES_PER_CONVERSION_PERIOD);
}

bool HX711::_isReady() noexcept {

    assert(this->_gpioHandle != -1);

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

    assert(this->_gpioHandle != -1);

    //First, clock pin is set high to make DOUT ready to be read from
    ::lgGpioWrite(this->_gpioHandle, this->_clockPin, 1);

    //Then delay for sufficient time to allow DOUT to be ready (0.1us)
    //this will also permit a sufficient amount of time for the clock
    //pin to remain high
    //
    //NOTE: in practice this [probably] isn't really going to matter
    //due to how miniscule the amount of time is and how slow the
    //execution of the code is in comparison
    _delayns(duration_cast<nanoseconds>(microseconds(1)));

    //At this stage, DOUT is ready and the clock pin has been held
    //high for sufficient amount of time, so read the bit value
    const bool bit = ::lgGpioRead(this->_gpioHandle, this->_dataPin) == 1;

    //The clock pin then needs to be held for at least 0.2us before
    //the next bit can be read
    //
    //NOTE: as before, the delay probably isn't going to matter
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

void HX711::_readRawBytes(BYTE* const bytes) {

    using namespace std::chrono;

    std::unique_lock<std::mutex> communication(this->_commLock);

    /**
     * When DOUT goes low, there is a minimum of 0.1us until the clock pin
     * can go high. T1 in Fig.2.
     * Datasheet pg. 5
     * 
     * NOTE: as described earlier, this [probably] isn't going to matter
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
    std::copy(std::begin(raw), std::end(raw), bytes);

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
    //lguSleep could also be used from the lgpio.h header
    std::this_thread::sleep_for(ns);
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
     * 
     * ALSO - as a consequence of timeval, the highest precision
     * available is microseconds. So, even though this function's
     * argument is std::chrono::nanoseconds, the smallest delay
     * is actually 1 microsecond.
     */

    using namespace std::chrono;

    struct timeval tNow;
    struct timeval tLong = {0};
    struct timeval tEnd;

    /**
     * There is no point setting tLong.tv_sec. This function only
     * delays for - at most - 100us. tv_sec will therefore always
     * be 0. But, to be on the safe side, when tLong is declared
     * it is 0-initialised.
     */
    tLong.tv_usec = duration_cast<microseconds>(ns).count() % 
        microseconds::period::den;

    ::gettimeofday(&tNow, nullptr);
    timeradd(&tNow, &tLong, &tEnd);

    //cppcheck-suppress syntaxError
    while(timercmp(&tNow, &tEnd, <)) {
        ::gettimeofday(&tNow, nullptr);
    }

}

void* HX711::_watchPin(void* const hx711ptr) {
    
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

    /**
     * TODO: can <atomic> be used to sync state?
     * 
     * Update 1: Perhaps not. Need finer-grained control over WHEN
     * the state can be modified. Mutex serves that purpose.
     */

    assert(hx711ptr != nullptr);

    HX711* const self = static_cast<HX711*>(hx711ptr);

    const pthread_t threadId = ::pthread_self();
    std::unique_lock<std::mutex> stateLock(self->_pinWatchLock, std::defer_lock);
    std::unique_lock<std::mutex> dataReadyLock(self->_readyLock, std::defer_lock);

    //scope for sched params to be used and removed from stack 
    {

        /**
         * Set the thread's policy to realtime and lower the priority to
         * minimum. Increase the priority once the thread state changes to normal.
         */
        struct sched_param schParams = {
            ::sched_get_priority_min(_PINWATCH_SCHED_POLICY)
        };

        /**
         * This may return...
         * 
         * EPERM  The caller does not have appropriate privileges to set the
         * specified scheduling policy and parameters.
         * https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html
         * 
         * If this occurs, is it still acceptable to continue at a reduced
         * priority? Yes. Use sudo if needed or calling code can temporarily
         * elevate permissions.
         */
        ::pthread_setschedparam(
            threadId,
            _PINWATCH_SCHED_POLICY,
            &schParams);

    }

    /**
     * Leaving this here for future readers.
     * 
     * Any processes or threads using SCHED_FIFO or SCHED_RR shall be 
     * unaffected by a call to setpriority().
     * https://linux.die.net/man/3/setpriority
     * 
     * ::setpriority(PRIO_PROCESS, 0, PRIO_MAX);
     */

    for(;;) {

        stateLock.lock();

        if(self->_watchState == PinWatchState::END) {
            break;
        }
        
        if(self->_watchState == PinWatchState::PAUSE) {
            stateLock.unlock();
            ::pthread_setschedprio(
                threadId,
                ::sched_get_priority_min(_PINWATCH_SCHED_POLICY));
            ::sched_yield();
            _sleepns(self->_pauseSleep);
            continue;
        }

        //thread is in normal state; increase priority to max
        ::pthread_setschedprio(
            threadId,
            ::sched_get_priority_max(_PINWATCH_SCHED_POLICY));

        if(!dataReadyLock.owns_lock()) {
            dataReadyLock.lock();
        }

        if(!self->_isReady()) {
            stateLock.unlock();
            //::sched_yield();
            _sleepns(self->_notReadySleep);
            continue;
        }

        const Value v = self->_readInt();

        if(!v.isValid()) {
            stateLock.unlock();
            //sleep if out of range
            //TODO: implement member for this
            //::sched_yield();
            //_sleepns();
            continue;
        }

        self->_lastVal = v;
        self->_dataReady.notify_all();
        dataReadyLock.unlock();
        stateLock.unlock();
        _sleepns(self->_pollSleep);

    }

    /**
     * Any thread cleanup stuff goes here, under the loop
     */
    ::pthread_exit(nullptr);

}

void HX711::_changeWatchState(const PinWatchState state) {
    std::lock_guard<std::mutex> lck(this->_pinWatchLock);
    this->_watchState = state;
}

HX711::HX711(const int dataPin, const int clockPin) noexcept :
    _gpioHandle(-1),
    _dataPin(dataPin),
    _clockPin(clockPin),

    /**
     * The intention here is for the initial value to be one which is
     * invalid. At construction, no values have been read, so the value
     * currently held should not be usabled. Value::isValid() will
     * therefore return false in this case.
     */
    _lastVal(Value()),
    _watchState(PinWatchState::PAUSE),
    _pauseSleep(_DEFAULT_PAUSE_SLEEP),
    _notReadySleep(_DEFAULT_NOT_READY_SLEEP),
    _pollSleep(_DEFAULT_POLL_SLEEP),

    /**
     * The HX711 chip's output rate will be at 10Hz when its X1 pin is
     * pulled to ground. On boards such as Sparkfun's HX711, this is the
     * default.
     */
    _rate(Rate::HZ_10),

    /**
     * Datasheet pg. 5 describes that after a reset, the HX711 will default
     * to channel A and a gain of 128.
     */
    _channel(Channel::A),
    _gain(Gain::GAIN_128),

    /**
     * Datasheet pg. 4 describes the HX711 outputting bits in MSB order.
     */
    _bitFormat(Format::MSB),
    _byteFormat(Format::MSB) {
}

HX711::~HX711() {
    this->_changeWatchState(PinWatchState::END);
    assert(this->_watchState == PinWatchState::END);
    ::lgGpioFree(this->_gpioHandle, this->_clockPin);
    ::lgGpioFree(this->_gpioHandle, this->_dataPin);
    ::lgGpiochipClose(this->_gpioHandle);
}

void HX711::begin() {

    /**
     * TODO: move this to constructor?
     * 
     * Update 1: This seems a bit too involved for a constructor with
     * exceptions possibly being thrown. I'm comfortable leaving it as
     * a separate begin() function as it is.
     */

    if(!(   
        (this->_gpioHandle = ::lgGpiochipOpen(0)) >= 0 &&
        ::lgGpioClaimInput(this->_gpioHandle, 0, this->_dataPin) == 0 &&
        ::lgGpioClaimOutput(this->_gpioHandle, 0, this->_clockPin, 0) == 0
    )) {
        throw std::runtime_error("unable to access GPIO");
    }

    pthread_t th;
    ::pthread_create(&th, nullptr, &HX711::_watchPin, this);
    ::pthread_detach(th);

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
        assert(this->_watchState == PinWatchState::NORMAL);

        for(size_t i = 0; i < len; ++i) {
            
            ready.lock();

            if(this->_dataReady.wait_for(ready, maxWait) == std::cv_status::timeout) {
                throw TimeoutException("timed out while trying to read from HX711");
            }

            arr[i] = this->_lastVal;
            ready.unlock();

        }

        this->_changeWatchState(PinWatchState::PAUSE);
        assert(this->_watchState == PinWatchState::PAUSE);

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

void HX711::setConfig(const Channel c, const Gain g, const Rate r) {

    //the rate is unrelated to channel and gain, so change it regardless
    //of whether the channel and gain combination is valid
    this->_rate = r;

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

void HX711::setFormat(const Format bitF, const Format byteF) noexcept {
    std::lock_guard<std::mutex> lock(this->_commLock);
    this->_bitFormat = bitF;
    this->_byteFormat = byteF;
}

void HX711::powerDown() noexcept {

    using namespace std::chrono;

    assert(this->_gpioHandle != -1);

    std::lock_guard<std::mutex> lock(this->_commLock);

    /**
     * The delay between low to high is probably not necessary, but it
     * should help to keep the underlying code from optimising it away -
     * if does at all.
     */
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

    using namespace std::chrono;

    assert(this->_gpioHandle != -1);

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

    /**
     * Settling time
     * Datasheet pg. 3
     */
    if(this->_rate == Rate::HZ_10) {
        _sleepns(duration_cast<nanoseconds>(milliseconds(400)));
    }
    else if(this->_rate == Rate::HZ_80) {
        _sleepns(duration_cast<nanoseconds>(milliseconds(50)));
    }

}

/**
 * Used to select to correct number of clock pulses depending on the
 * gain
 * Datasheet pg. 4
 */
const std::unordered_map<const Gain, const std::uint8_t> HX711::_PULSES({
    { Gain::GAIN_128, 25 },
    { Gain::GAIN_32,  26 },
    { Gain::GAIN_64,  27 }
});

};