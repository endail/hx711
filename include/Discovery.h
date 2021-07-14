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

namespace HX711 {
class Discovery : public HX711 {

public:
    Discovery(const int dataPin, const int clockPin) : 
        HX711(dataPin, clockPin) {
            this->begin();
    }

    std::vector<std::chrono::nanoseconds> getTimeToReady(const std::size_t samples) {

        using namespace std::chrono;

        std::vector<nanoseconds> timings;
        timings.reserve(samples);

        //busy-wait
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