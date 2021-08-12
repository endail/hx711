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
#include <vector>
#include "../include/HX711.h"
#include "../include/Mass.h"
#include "../include/SimpleHX711.h"
#include "../include/Value.h"

namespace HX711 {

SimpleHX711::SimpleHX711(
    const int dataPin,
    const int clockPin,
    const Value refUnit,
    const Value offset,
    const Rate rate) :
        AbstractScale(Mass::Unit::G, refUnit, offset),
        HX711(dataPin, clockPin, rate) {
            this->begin();
}

std::vector<Value> SimpleHX711::getValues(const std::chrono::nanoseconds timeout) {

    using namespace std::chrono;

    std::vector<Value> vals;
    const auto endTime = high_resolution_clock::now() + timeout;

    while(high_resolution_clock::now() < endTime) {
        while(!this->isReady());
        vals.push_back(this->readValue());
    }

    return vals;

}

std::vector<Value> SimpleHX711::getValues(const std::size_t samples) {
    
    if(samples == 0) {
        throw std::range_error("samples must be at least 1");
    }
    
    std::vector<Value> vals;
    vals.reserve(samples);
    
    for(std::size_t i = 0; i < samples; ++i) {
        while(!this->isReady());
        vals.push_back(this->readValue());
    }
    
    return vals;

}

};
