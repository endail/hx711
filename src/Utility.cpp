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
#include <lgpio.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <thread>
#include <time.h>
#include "../include/GpioException.h"
#include "../include/Utility.h"

namespace HX711 {

constexpr const char* const Utility::_VERSION;

void Utility::_throwGpioExIfErr(const int code) {
    if(code < 0) {
        throw GpioException(::lguErrorText(code));
    }
}

const char* Utility::getVersion() noexcept {
    return _VERSION;
}

int Utility::openGpioHandle(const int chip) {
    const auto code = ::lgGpiochipOpen(chip);
    _throwGpioExIfErr(code);
    return code;
}

void Utility::closeGpioHandle(const int chip) {
    _throwGpioExIfErr(::lgGpiochipClose(chip));
}

void Utility::openGpioInput(const int handle, const int pin) {
    _throwGpioExIfErr(::lgGpioClaimInput(handle, LG_SET_PULL_UP, pin));
}

void Utility::openGpioOutput(const int handle, const int pin) {
    _throwGpioExIfErr(::lgGpioClaimOutput(handle, 0, pin, 0));
}

void Utility::closeGpioPin(const int handle, const int pin) {
    _throwGpioExIfErr(::lgGpioFree(handle, pin));
}

GpioLevel Utility::readGpio(const int handle, const int pin) {
    const auto code = ::lgGpioRead(handle, pin);
    _throwGpioExIfErr(code);
    //lgGpioRead returns 0 for low and 1 for high
    //underlying GpioLevel is bool type
    return static_cast<GpioLevel>(code);
}

void Utility::writeGpio(const int handle, const int pin, const GpioLevel lev) {
    _throwGpioExIfErr(::lgGpioWrite(handle, pin, static_cast<int>(lev)));
}

void Utility::sleep(const std::chrono::nanoseconds ns) noexcept {
    std::this_thread::sleep_for(ns);
}

void Utility::delay(const std::chrono::nanoseconds ns) noexcept {

    using namespace std::chrono;

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

    /**
     * TODO: figure out the overhead in calling this function
     */

    struct timespec tNow;
    struct timespec tLong;
    struct timespec tEnd;

    tLong.tv_sec = ns.count() / nanoseconds::period::den;
    tLong.tv_nsec = ns.count() % nanoseconds::period::den;

    ::clock_gettime(CLOCK_MONOTONIC_RAW, &tNow);
    timespecadd(&tNow, &tLong, &tEnd);

    while(timespeccmp(&tNow, &tEnd) < 0) {
        ::clock_gettime(CLOCK_MONOTONIC_RAW, &tNow);
    }

}

std::chrono::nanoseconds Utility::getnanos() noexcept {
    timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return timespec_to_nanos(&ts);
}

std::chrono::nanoseconds Utility::timespec_to_nanos(const timespec* const ts) noexcept {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(seconds(ts->tv_sec)) + nanoseconds(ts->tv_nsec);
}

void Utility::timespecclear(timespec* const tsp) noexcept {
    tsp->tv_sec = tsp->tv_nsec = 0;
}

bool Utility::timespecisset(const timespec* const tsp) noexcept {
    return tsp->tv_sec || tsp->tv_nsec;
}

bool Utility::timespecisvalid(const timespec* const tsp) noexcept {
    using namespace std::chrono;
    return tsp->tv_nsec >= 0 && tsp->tv_nsec < nanoseconds::period::den;
}

int Utility::timespeccmp(const timespec* const tsp, const timespec* const usp) noexcept {

    if(tsp->tv_sec < usp->tv_sec) {
        return -1;
    }
    else if(tsp->tv_sec > usp->tv_sec) {
        return 1;
    }

    if(tsp->tv_nsec < usp->tv_nsec) {
        return -1;
    }
    else if(tsp->tv_nsec > usp->tv_nsec) {
        return 1;
    }

    return 0;

}

void Utility::timespecadd(const timespec* const tsp, const timespec* const usp, timespec* const vsp) noexcept {

    using namespace std::chrono;

    vsp->tv_sec = tsp->tv_sec + usp->tv_sec;
    vsp->tv_nsec = tsp->tv_nsec + usp->tv_nsec;

    if(vsp->tv_nsec >= nanoseconds::period::den) {
        vsp->tv_sec++;
        vsp->tv_nsec -= nanoseconds::period::den;
    }

}

void Utility::timespecsub(const timespec* const tsp, const timespec* const usp, timespec* const vsp) noexcept {

    using namespace std::chrono;

    vsp->tv_sec = tsp->tv_sec - usp->tv_sec;
    vsp->tv_nsec = tsp->tv_nsec - usp->tv_nsec;

    if(vsp->tv_nsec < 0) {
        vsp->tv_sec--;
        vsp->tv_nsec += nanoseconds::period::den;
    }

}

void Utility::setThreadPriority(const int pri, const int policy, const pthread_t th) noexcept {

    struct sched_param schParams = {
        pri
    };

    /**
     * Leaving this here for future readers.
     * 
     * Any processes or threads using SCHED_FIFO or SCHED_RR shall be 
     * unaffected by a call to setpriority().
     * https://linux.die.net/man/3/setpriority
     * 
     * ::setpriority(PRIO_PROCESS, 0, PRIO_MAX);
     */

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
        th,
        policy,
        &schParams);

}

};
