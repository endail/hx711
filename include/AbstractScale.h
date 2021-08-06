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

#ifndef HX711_SCALE_H_5E9DF993_BC93_4934_A844_98355F4F8062
#define HX711_SCALE_H_5E9DF993_BC93_4934_A844_98355F4F8062

#include <cstdint>
#include <vector>
#include "Mass.h"
#include "Value.h"

namespace HX711 {

enum class ReadType {
    Median,
    Average
};

class AbstractScale {

protected:
    Mass::Unit _massUnit;
    Value _refUnit;
    Value _offset;

public:
    AbstractScale(
        const Mass::Unit massUnit,
        const Value refUnit,
        const Value offset) noexcept;

    void setUnit(const Mass::Unit unit) noexcept;
    Mass::Unit getUnit() const noexcept;

    Value getReferenceUnit() const noexcept;
    void setReferenceUnit(const Value refUnit);

    Value getOffset() const noexcept;
    void setOffset(const Value offset) noexcept;

    double normalise(const double v) const noexcept;

    virtual std::vector<Value> getValues(const std::size_t samples) = 0;

    double read(const ReadType rt = ReadType::Median, const std::size_t samples = 3);
    void zero(const ReadType rt = ReadType::Median, const std::size_t samples = 3);
    Mass weight(const ReadType rt = ReadType::Median, const std::size_t samples = 3);

};
};
#endif
