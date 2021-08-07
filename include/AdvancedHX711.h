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

#ifndef HX711_ADVANCEDHX711_H_16C895F6_FD48_4E45_B690_97EFE239A14E
#define HX711_ADVANCEDHX711_H_16C895F6_FD48_4E45_B690_97EFE239A14E

#include <chrono>
#include <cstdint>
#include <sched.h>
#include "AbstractScale.h"
#include "HX711.h"
#include "Value.h"
#include "Watcher.h"

namespace HX711 {
class AdvancedHX711 : public AbstractScale, public HX711 {

protected:
    Watcher* _wx;

public:
    AdvancedHX711(
        const int dataPin,
        const int clockPin,
        const Value refUnit = 1,
        const Value offset = 0);

    virtual ~AdvancedHX711();

    virtual std::vector<Value> getValues(const std::chrono::nanoseconds timeout) override;
    virtual std::vector<Value> getValues(const std::size_t samples) override;

};
};
#endif
