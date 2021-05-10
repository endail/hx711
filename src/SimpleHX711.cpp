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

#include "../include/SimpleHX711.h"
#include "../include/HX711.h"
#include "../include/Mass.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>

namespace HX711 {

std::vector<HX_VALUE> SimpleHX711::_readValues(const std::size_t times) {

    std::vector<HX_VALUE> vals;
    vals.reserve(times);

    for(std::size_t i = 0; i < times; ++i) {
        vals.push_back(this->_hx->getValue(this->_ch));
    }

    return vals;

}

double SimpleHX711::_median(const std::vector<HX_VALUE>* vals) {

    if(vals == nullptr || vals->empty()) {
        throw std::invalid_argument("vals is null or empty");
    }

    //to calculate the median the vector needs to be modifiable
    //hence, a copy is made
    std::vector<HX_VALUE> copied = *vals;

    //https://stackoverflow.com/a/42791986/570787
    if(copied.size() % 2 == 0) {

        const auto median_it1 = copied.begin() + copied.size() / 2 - 1;
        const auto median_it2 = copied.begin() + copied.size() / 2;

        std::nth_element(copied.begin(), median_it1, copied.end());
        const auto e1 = *median_it1;

        std::nth_element(copied.begin(), median_it2, copied.end());
        const auto e2 = *median_it2;

        return (e1 + e2) / 2.0;

    }
    else {
        const auto median_it = copied.begin() + copied.size() / 2;
        std::nth_element(copied.begin(), median_it, copied.end());
        return static_cast<double>(*median_it);
    }

}

double SimpleHX711::_average(const std::vector<HX_VALUE>* vals) {

    if(vals == nullptr || vals->empty()) {
        throw std::invalid_argument("vals is null or empty");
    }

    const std::int64_t sum = std::accumulate(
        vals->begin(), vals->end(), 0);

    return static_cast<double>(sum) / vals->size();

}

SimpleHX711::SimpleHX711(const SimpleHX711& s2) noexcept {
}

SimpleHX711& SimpleHX711::operator=(const SimpleHX711& rhs) noexcept {
    return *this;
}

SimpleHX711::SimpleHX711(
    const int dataPin,
    const int clockPin,
    const HX_VALUE refUnit,
    const HX_VALUE offset) :
        _refUnit(refUnit),
        _offset(offset)  {
            this->_hx = new HX711(dataPin, clockPin);
            this->_hx->connect();
}

SimpleHX711::~SimpleHX711() {
    delete this->_hx;
}

SimpleHX711::operator bool() const noexcept {
    return this->_hx->isReady();
}

bool operator!(const SimpleHX711& hx) noexcept {
    return !hx._hx->isReady();
}

void SimpleHX711::setUnit(const Mass::Unit unit) noexcept {
    this->_scaleUnit = unit;
}

Mass::Unit SimpleHX711::getUnit() const noexcept {
    return this->_scaleUnit;
}

HX_VALUE SimpleHX711::getReferenceUnit() const noexcept {
    return this->_refUnit;
}

void SimpleHX711::setReferenceUnit(const HX_VALUE refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_refUnit = refUnit;

}

HX_VALUE SimpleHX711::getOffset() const noexcept {
    return this->_offset;
}

void SimpleHX711::setOffset(const HX_VALUE offset) noexcept {
    this->_offset = offset;
}

void SimpleHX711::setChannel(const Channel ch) noexcept {
    this->_ch = ch;
}

Channel SimpleHX711::getChannel() const noexcept {
    return this->_ch;
}

HX711* SimpleHX711::getBase() noexcept {
    return this->_hx;
}

void SimpleHX711::tare(const ReadType r, const size_t times) {
    const HX_VALUE backup = this->_refUnit;
    this->setReferenceUnit(1);
    this->_offset = static_cast<HX_VALUE>(std::round(this->read(r, times)));
    this->setReferenceUnit(backup);
}

Mass SimpleHX711::weight(const ReadType r, const size_t times) {
    return Mass(this->read(r, times), this->_scaleUnit);
}

double SimpleHX711::read(const ReadType r, const std::size_t times) {

    if(times == 0) {
        throw std::range_error("times must be at least 1");
    }

    double val;

    std::vector<HX_VALUE> vals = this->_readValues(times);

    switch(r) {
        case ReadType::Median:
            val = _median(&vals);
            break;
        case ReadType::Average:
            val = _average(&vals);
            break;
        default:
            throw std::invalid_argument("unknown read type");
    }

    return (val - this->_offset) / this->_refUnit;

}

};
