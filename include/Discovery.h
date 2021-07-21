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

#ifndef HX711_DISCOVERY_H_82654AF1_3AA3_4885_9BE7_FA38117DE328
#define HX711_DISCOVERY_H_82654AF1_3AA3_4885_9BE7_FA38117DE328

#include "HX711.h"
#include <chrono>
#include <vector>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <algorithm>
#include <cmath>
#include <gsl/gsl_statistics.h>

namespace HX711 {

using namespace std::chrono;

struct TimingResult {
public:
    Value v;
    high_resolution_clock::time_point start;
    high_resolution_clock::time_point waitStart;
    high_resolution_clock::time_point waitEnd;
    high_resolution_clock::time_point convertStart;
    high_resolution_clock::time_point convertEnd;
    high_resolution_clock::time_point end;

    std::chrono::microseconds getWaitTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->waitEnd - this->waitStart);
    }

    std::chrono::microseconds getConversionTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->convertEnd - this->convertStart);
    }

    std::chrono::microseconds getTotalTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            this->end - this->start);
    }

};

class TimingCollection : public std::vector<TimingResult> {
public:
    struct Stats {
        double min;
        double max;
        double med;
        double std;

        bool inRange(const double n) const noexcept {
            const double minDev = this->med - this->std;
            const double maxDev = this->med + this->std;
            return n >= minDev && n <= maxDev;
        }

    };

protected:

    Stats _statsFromVector(std::vector<double>& vec) const noexcept {

        Stats s;

        std::sort(vec.begin(), vec.end());

        s.min = gsl_stats_min(vec.data(), 1, vec.size());
        s.max = gsl_stats_max(vec.data(), 1, vec.size());
        s.med = gsl_stats_median_from_sorted_data(vec.data(), 1, vec.size());

        //double work[vec.size()];
        s.std = gsl_stats_sd(vec.data(), 1, vec.size());

        return s;

    }

public:

    Stats getWaitTimeStats() const noexcept {

        std::vector<double> vec;
        vec.reserve(this->size());

        for(auto it = this->cbegin(); it != this->cend(); ++it) {
            vec.push_back(static_cast<double>(it->getWaitTime().count()));
        }

        return _statsFromVector(vec);

    }

    Stats getConversionTimeStats() const noexcept {

        std::vector<double> vec;
        vec.reserve(this->size());

        for(auto it = this->cbegin(); it != this->cend(); ++it) {
            vec.push_back(static_cast<double>(it->getConversionTime().count()));
        }

        return _statsFromVector(vec);

    }

    Stats getTotalTimeStats() const noexcept {

        std::vector<double> vec;
        vec.reserve(this->size());

        for(auto it = this->cbegin(); it != this->cend(); ++it) {
            vec.push_back(static_cast<double>(it->getTotalTime().count()));
        }

        return _statsFromVector(vec);

    }

};

class Discovery : public HX711 {
public:

    Discovery(const int dataPin, const int clockPin, const Rate rate) : 
        HX711(dataPin, clockPin, rate) {
            this->begin();
    }

    TimingCollection getTimings(const std::size_t samples) {

        using namespace std::chrono;

        TimingCollection vec;
        vec.reserve(samples);

        for(size_t i = 0; i < samples; ++i) {

            TimingResult tr;

            tr.start = high_resolution_clock::now();

            tr.waitStart = high_resolution_clock::now();
            while(!this->_isReady()) ;
            tr.waitEnd = high_resolution_clock::now();

            tr.convertStart = high_resolution_clock::now();
            tr.v = this->_readInt();
            tr.convertEnd = high_resolution_clock::now();

            tr.end = high_resolution_clock::now();

            vec.push_back(tr);

        }

        return vec;

    }


    std::vector<std::chrono::nanoseconds> getTimeToReady(const std::size_t samples) {

        using namespace std::chrono;

        std::vector<nanoseconds> timings;
        timings.reserve(samples);

        //do an initial read
        while(!this->_isReady()) ;
        this->_readInt();

        for(std::size_t i = 0; i < samples; ++i) {

            high_resolution_clock::time_point start = high_resolution_clock::now();
            while(!this->_isReady()) ;
            high_resolution_clock::time_point end = high_resolution_clock::now();

            timings.push_back(end - start);

            this->_readInt();

        }

        return timings;

    }

};
};
#endif