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
#include "../include/ValueStack.h"

namespace HX711 {

constexpr std::chrono::nanoseconds ValueStack::_DEFAULT_MAX_AGE;

void ValueStack::_update() {

    using namespace std::chrono;

    while(this->_container.size() > this->_maxSize) {
        this->_container.pop_back();
    }

    const auto now = steady_clock::now();

    this->_container.remove_if([this, &now](const StackEntry& e) {
        return (e.when + this->_maxAge) > now;
    });

}

ValueStack::ValueStack(
    const std::size_t maxSize,
    const std::chrono::nanoseconds maxAge) noexcept :
        _maxSize(maxSize),
        _maxAge(maxAge) {
}

void ValueStack::push(const std::int32_t val) noexcept {

    this->_update();

    if(this->full()) {
        this->_container.pop_back();
    }

    StackEntry e;
    e.val = val;
    e.when = std::chrono::steady_clock::now();

    this->_container.push_front(e);

}

std::int32_t ValueStack::pop() noexcept {
    const std::int32_t v = this->_container.front().val;
    this->_container.pop_front();
    return v;
}

std::size_t ValueStack::size() const noexcept {
    return this->_container.size();
}

void ValueStack::clear() noexcept {
    this->_container.clear();
}

bool ValueStack::empty() const noexcept {
    return this->_container.empty();
}

bool ValueStack::full() const noexcept {
    return this->_container.size() >= this->_maxSize;
}

};
