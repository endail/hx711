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
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <limits>
#include <numeric>
#include <pthread.h>
#include <time.h>
#include <vector>

namespace HX711 {

enum class GpioLevel : bool {
    LOW = false,
    HIGH = true
};

class Utility {
protected:
    static void _throwGpioExIfErr(const int code);
    Utility();


public:

    static int openGpioHandle(const int chip);
    static void closeGpioHandle(const int chip);
    static void openGpioInput(const int handle, const int pin);
    static void openGpioOutput(const int handle, const int pin);
    static void closeGpioPin(const int handle, const int pin);
    static GpioLevel readGpio(const int handle, const int pin);
    static void writeGpio(const int handle, const int pin, const GpioLevel lev);

    /**
     * Sleep for ns nanoseconds. The _sleepns/_delayns functions are
     * an attempt to be analogous to usleep/udelay in the kernel.
     * https://www.kernel.org/doc/html/v5.10/timers/timers-howto.html
     */
    static void sleepns(const std::chrono::nanoseconds ns) noexcept;
    static void delayus(const std::chrono::microseconds us) noexcept;
    static void delayns(const std::chrono::nanoseconds ns) noexcept;
    static std::chrono::nanoseconds getnanos() noexcept;

    static std::chrono::nanoseconds timespec_to_nanos(const timespec* const ts) noexcept;

    static void timespecclear(timespec* const tsp) noexcept;
    static bool timespecisset(const timespec* const tsp) noexcept;
    static bool timespecisvalid(const timespec* const tsp) noexcept;
    static int timespeccmp(const timespec* const tsp, const timespec* const usp) noexcept;
    static void timespecadd(const timespec* const tsp, const timespec* const usp, timespec* const vsp) noexcept;
    static void timespecsub(const timespec* const tsp, const timespec* const usp, timespec* const vsp) noexcept;

    static void setThreadPriority(
        const int pri, const int policy, const pthread_t th) noexcept;

    template <typename T>
    static double average(const std::vector<T>* const vals) noexcept {

        assert(vals != nullptr);
        assert(!vals->empty());

        const long long int sum = std::accumulate(
            vals->begin(), vals->end(), static_cast<long long int>(0));

        return static_cast<double>(sum) / vals->size();

    }

    template <typename T>
    static double median(std::vector<T>* const vals) noexcept {

        assert(vals != nullptr);
        assert(!vals->empty());

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

    //reverse bits in int
    //https://stackoverflow.com/a/2602871/570787
    template <typename T>
    static T reverse(T n, size_t b = sizeof(T) * CHAR_BIT) noexcept {

        assert(b <= std::numeric_limits<T>::digits);

        T rv = 0;

        for (size_t i = 0; i < b; ++i, n >>= 1) {
            rv = (rv << 1) | (n & 0x01);
        }

        return rv;

    }

};
};
#endif
