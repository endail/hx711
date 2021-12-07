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

#include <limits>
#include "../include/Value.h"

namespace HX711 {

Value::operator val_t() const noexcept {
    return this->_v;
}

bool Value::isSaturated() const noexcept {
    return this->_v == SATURATION_MIN || this->_v == SATURATION_MAX;
}

bool Value::isValid() const noexcept {
    return this->_v >= _MIN && this->_v <= _MAX;
}

Value::Value(const val_t v) noexcept : _v(v) {
}

Value::Value() noexcept : _v(std::numeric_limits<val_t>::min()) {
}

Value& Value::operator=(const Value& v2) noexcept {
    this->_v = v2._v;
    return *this;
}

};
