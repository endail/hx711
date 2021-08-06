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

#ifndef HX711_VALUE_H_48F704C7_6B2D_4BEC_9599_62045594695B
#define HX711_VALUE_H_48F704C7_6B2D_4BEC_9599_62045594695B

#include <cstdint>

namespace HX711 {
class Value {

protected:

    /**
     * TODO: is it feasible to use a three byte array instead of a
     * four byte int? 
     */

    typedef std::int32_t _INTERNAL_TYPE;

    _INTERNAL_TYPE _v;

    /**
     * Datasheet pg. 3
     * But also a consequence of the sensor being 24 bits
     */
    static const _INTERNAL_TYPE _MIN = -0x800000;
    static const _INTERNAL_TYPE _MAX = 0x7FFFFF;


public:

    /**
     * When input differential signal goes out of
     * the 24 bit range, the output data will be saturated
     * at 800000h (MIN) or 7FFFFFh (MAX), until the
     * input signal comes back to the input range.
     * Datasheet pg. 4
     */
    bool isSaturated() const noexcept;

    /**
     * A 32 bit integer holds the actual value from the sensor. Calling
     * this function makes sure it is within the 24 bit range used by
     * the sensor.
     */
    bool isValid() const noexcept;
    operator _INTERNAL_TYPE() const noexcept;
    
    //cppcheck-suppress noExplicitConstructor
    Value(const _INTERNAL_TYPE v) noexcept;
    Value() noexcept;
    Value& operator=(const Value& v2) noexcept;

};
};
#endif
