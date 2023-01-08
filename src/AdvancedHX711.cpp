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
#include <stdexcept>
#include <thread>
#include <vector>
#include "../include/AdvancedHX711.h"
#include "../include/HX711.h"
#include "../include/Mass.h"
#include "../include/Utility.h"
#include "../include/Watcher.h"

namespace HX711 {

AdvancedHX711::AdvancedHX711(
    const int dataPin,
    const int clockPin,
    const int refUnit,
    const int offset,
    const Rate rate) : 
        AbstractScale(Mass::Unit::G, refUnit, offset),
        HX711(dataPin, clockPin, rate) {
            this->_wx = new Watcher(this);
            this->_wx->begin();
            this->connect();
}

AdvancedHX711::~AdvancedHX711() {
    delete this->_wx;
}

std::vector<std::int32_t> AdvancedHX711::getValues(const std::chrono::nanoseconds timeout) {

    using namespace std::chrono;

    this->_wx->values.clear();
    this->_wx->watch();

    std::vector<std::int32_t> vals;
    const auto endTime = steady_clock::now() + timeout;

    while(true) {

        if(steady_clock::now() >= endTime) {
            this->_wx->pause();
            return vals;
        }

        if(!this->_wx->values.empty()) {

            this->_wx->valuesLock.lock();
            while(!this->_wx->values.empty()) {
                vals.push_back(this->_wx->values.pop());
            }
            this->_wx->valuesLock.unlock();

            continue;

        }

        std::this_thread::yield();
        Utility::sleep(milliseconds(1));

    }

}

std::vector<std::int32_t> AdvancedHX711::getValues(const std::size_t samples) {

    using namespace std::chrono;

    if(samples == 0) {
        throw std::range_error("samples must be at least 1");
    }

    this->_wx->values.clear();
    this->_wx->watch();

    /**
     * TODO: is lowering this thread's priority feasible?
     * ie. in favour of increasing the watcher's priority
     */

    std::vector<std::int32_t> vals;
    vals.reserve(samples);

    //while not filled
    while(vals.size() < samples) {

        //while empty, defer exec to other threads
        while(this->_wx->values.empty()) {
            std::this_thread::yield();
            Utility::sleep(milliseconds(1));
        }

        //not empty; data available!
        this->_wx->valuesLock.lock();

        //now, take as many values as which are available
        //up to however many are left to fill the array
        while(!this->_wx->values.empty() && vals.size() < samples) {
            vals.push_back(this->_wx->values.pop());
        }

        this->_wx->valuesLock.unlock();

    }

    this->_wx->pause();

    return vals;

}

};
