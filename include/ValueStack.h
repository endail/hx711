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

#ifndef HX711_VALUESTACK_H_609AB3D7_0D6C_45EE_8864_5A1D5E866ACA
#define HX711_VALUESTACK_H_609AB3D7_0D6C_45EE_8864_5A1D5E866ACA

#include <chrono>
#include <cstdint>
#include <list>

namespace HX711 {
class ValueStack {
protected:

    struct StackEntry {
        Value val;
        std::chrono::steady_clock::time_point when;
    };

    static const size_t _DEFAULT_MAX_SIZE = 80;
    static constexpr auto _DEFAULT_MAX_AGE = std::chrono::duration_cast
        <std::chrono::nanoseconds>(std::chrono::seconds(1));

    void _update();

    std::list<StackEntry> _container;
    std::size_t _maxSize;
    std::chrono::nanoseconds _maxAge;

public:

    ValueStack(
        const std::size_t maxSize = _DEFAULT_MAX_SIZE,
        const std::chrono::nanoseconds maxAge = _DEFAULT_MAX_AGE) noexcept;

    void push(const std::int32_t val) noexcept;
    std::int32_t pop() noexcept;
    std::size_t size() const noexcept;
    void clear() noexcept;
    bool empty() const noexcept;
    bool full() const noexcept;

};
};
#endif
