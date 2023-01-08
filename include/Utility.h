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

#ifndef HX711_UTILITY_H_2F1DEBBD_F7BB_4202_BE2A_A33018D2AF5A
#define HX711_UTILITY_H_2F1DEBBD_F7BB_4202_BE2A_A33018D2AF5A

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <limits>
#include <numeric>
#include <pthread.h>
#include <stdexcept>
#include <time.h>
#include <vector>

namespace HX711 {

enum class GpioLevel : bool {
    LOW = false,
    HIGH = true
};

class Utility {
protected:

    static constexpr const char* const _VERSION = "2.11.0";

    static inline void _throwGpioExIfErr(const int code) {
        if(code < 0) {
            throw GpioException(::lguErrorText(code));
        }
    }

    Utility();


public:

    static inline const char* getVersion() noexcept {
        return _VERSION;
    }

    static inline int openGpioHandle(const int chip) {
        const auto code = ::lgGpiochipOpen(chip);
        _throwGpioExIfErr(code);
        return code;
    }

    static inline void closeGpioHandle(const int chip) {
        _throwGpioExIfErr(::lgGpiochipClose(chip));
    }

    static inline void openGpioInput(
        const int handle,
        const int pin) {
            _throwGpioExIfErr(::lgGpioClaimInput(handle, LG_SET_PULL_UP, pin));
    }

    static inline void openGpioOutput(
        const int handle,
        const int pin) {
            _throwGpioExIfErr(::lgGpioClaimOutput(handle, 0, pin, 0));
    }

    static inline void closeGpioPin(
        const int handle,
        const int pin) {
            _throwGpioExIfErr(::lgGpioFree(handle, pin));
    }

    static inline GpioLevel readGpio(
        const int handle,
        const int pin) {

            const auto code = ::lgGpioRead(handle, pin);
            _throwGpioExIfErr(code);
            //lgGpioRead returns 0 for low and 1 for high
            //underlying GpioLevel is bool type
            return static_cast<GpioLevel>(code);

    }

    static inline void writeGpio(
        const int handle,
        const int pin,
        const GpioLevel lev) {
            _throwGpioExIfErr(::lgGpioWrite(handle, pin, static_cast<int>(lev)));
    }

    /**
     * Sleep for ns nanoseconds. The _sleep/_delay functions are
     * an attempt to be analogous to usleep/udelay in the kernel.
     * https://www.kernel.org/doc/html/v5.10/timers/timers-howto.html
     */
    static inline void sleep(const std::chrono::nanoseconds ns) noexcept {
        std::this_thread::sleep_for(ns);
    }

    static void delay(const std::chrono::nanoseconds ns) noexcept;

    static inline std::chrono::nanoseconds getnanos() noexcept {
        timespec ts;
        ::clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        return timespec_to_nanos(&ts);
    }

    static inline std::chrono::nanoseconds timespec_to_nanos(const timespec* const ts) noexcept {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(seconds(ts->tv_sec)) + nanoseconds(ts->tv_nsec);
    }

    static inline void timespecclear(timespec* const tsp) noexcept {
        tsp->tv_sec = tsp->tv_nsec = 0;
    }

    static inline bool timespecisset(const timespec* const tsp) noexcept {
        return tsp->tv_sec || tsp->tv_nsec;
    }

    static inline bool timespecisvalid(const timespec* const tsp) noexcept {
        using namespace std::chrono;
        return tsp->tv_nsec >= 0 && tsp->tv_nsec < nanoseconds::period::den;
    }

    static int timespeccmp(
        const timespec* const tsp,
        const timespec* const usp) noexcept;

    static void timespecadd(
        const timespec* const tsp,
        const timespec* const usp,
        timespec* const vsp) noexcept;

    static void timespecsub(
        const timespec* const tsp,
        const timespec* const usp,
        timespec* const vsp) noexcept;

    static void setThreadPriority(
        const int pri,
        const int policy,
        const pthread_t th) noexcept;

    template <typename T>
    static double average(const std::vector<T>* const vals) noexcept {

        if(vals->empty()) {
            std::length_error("average cannot be calculated from 0 values");
        }

        const auto sum = std::accumulate(
            vals->begin(), vals->end(), static_cast<long long int>(0));

        return static_cast<double>(sum) / vals->size();

    }

    template <typename T>
    static double median(std::vector<T>* const vals) noexcept {

        if(vals->empty()) {
            std::length_error("median cannot be calculated from 0 values");
        }

        /**
         * TODO: is this more efficient?
         */
        if(vals->size() == 1) {
            return static_cast<double>((*vals)[0]);
        }

        //https://stackoverflow.com/a/42791986/570787
        if(vals->size() % 2 == 0) {

            const auto median_it1 = vals->begin() + vals->size() / 2 - 1;
            const auto median_it2 = vals->begin() + vals->size() / 2;

            std::nth_element(vals->begin(), median_it1, vals->end());
            const auto e1 = *median_it1;

            std::nth_element(vals->begin(), median_it2, vals->end());
            const auto e2 = *median_it2;

            return (e1 + e2) / 2.0;

        }
        else {
            const auto median_it = vals->begin() + vals->size() / 2;
            std::nth_element(vals->begin(), median_it, vals->end());
            return static_cast<double>(*median_it);
        }

    }

};

};
#endif
