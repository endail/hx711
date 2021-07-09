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
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>

namespace HX711 {

double SimpleHX711::_median(const std::vector<Value>* vals) {

    assert(vals != nullptr);
    assert(!vals->empty());

    if(vals->size() == 1) {
        return static_cast<double>((*vals)[0]);
    }

    //to calculate the median the vector needs to be modifiable
    //hence, a copy is made
    std::vector<Value> copied = *vals;

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

double SimpleHX711::_average(const std::vector<Value>* vals) {

    assert(vals != nullptr);
    assert(!vals->empty());

    const long long int sum = std::accumulate(
        vals->begin(), vals->end(), 0);

    return static_cast<double>(sum) / vals->size();

}

SimpleHX711::SimpleHX711(const SimpleHX711 &shx)
    _hx(shx._hx),
    _scaleUnit(shx._scaleUnit),
    _refUnit(shx._refUnit),
    _offset(shx._offset) {
}

SimpleHX711::operator=(const SimpleHX711& shx) {
    this->_hx = shx._hx;
    this->_scaleUnit = shx._scaleUnit;
    this->_refUnit = shx._refUnit;
    this->_offset = shx._offset;
}

SimpleHX711::SimpleHX711(
    const int dataPin,
    const int clockPin,
    const Value refUnit,
    const Value offset) :
        _hx(nullptr),
        _scaleUnit(Mass::Unit::G),
        _refUnit(refUnit),
        _offset(offset)  {
            this->_hx = new HX711(dataPin, clockPin);
            this->_hx->begin();
}

SimpleHX711::~SimpleHX711() {
    delete this->_hx;
}

void SimpleHX711::setUnit(const Mass::Unit unit) noexcept {
    this->_scaleUnit = unit;
}

Mass::Unit SimpleHX711::getUnit() const noexcept {
    return this->_scaleUnit;
}

Value SimpleHX711::getReferenceUnit() const noexcept {
    return this->_refUnit;
}

void SimpleHX711::setReferenceUnit(const Value refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_refUnit = refUnit;

}

Value SimpleHX711::getOffset() const noexcept {
    return this->_offset;
}

void SimpleHX711::setOffset(const Value offset) noexcept {
    this->_offset = offset;
}

double SimpleHX711::normalise(const double v) const noexcept {
    assert(this->_refUnit != 0);
    return (v - this->_offset) / this->_refUnit;
}

HX711* const SimpleHX711::getBase() noexcept {
    return this->_hx;
}

std::vector<Value> SimpleHX711::readValues(const std::size_t samples) {
    
    if(samples == 0) {
        throw std::range_error("samples must be at least 1");
    }
    
    std::vector<Value> vals;
    vals.resize(samples);
    this->_hx->getValues(vals.data(), samples);
    
    return vals;

}

void SimpleHX711::tare(const ReadType r, const size_t samples) {
    
    if(samples == 0) {
        throw std::range_error("samples must be at least 1");
    }
    
    const Value backup = this->_refUnit;
    this->setReferenceUnit(1);
    this->_offset = static_cast<Value>(std::round(this->read(r, samples)));
    this->setReferenceUnit(backup);
    
}

Mass SimpleHX711::weight(const ReadType r, const size_t samples) {
    return Mass(this->read(r, samples), this->_scaleUnit);
}

double SimpleHX711::read(const ReadType r, const std::size_t samples) {

    if(samples == 0) {
        throw std::range_error("samples must be at least 1");
    }

    double val;

    std::vector<Value> vals = this->readValues(samples);

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

    return this->normalise(val);

}

};
