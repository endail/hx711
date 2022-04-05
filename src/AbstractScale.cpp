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
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "../include/AbstractScale.h"
#include "../include/Mass.h"
#include "../include/Utility.h"
#include "../include/Value.h"

namespace HX711 {

Options::Options() noexcept
    : Options(_DEFAULT_SAMPLE_COUNT) { } //default constructor delegated to another

Options::Options(const std::size_t s, const ReadType rt) noexcept
    :   stratType(StrategyType::Samples),
        readType(rt),
        samples(s),
        timeout(0) { }

Options::Options(const std::chrono::nanoseconds t, const ReadType rt) noexcept
    :   stratType(StrategyType::Time),
        readType(rt),
        samples(0),
        timeout(t) { }

AbstractScale::AbstractScale(
    const Mass::Unit massUnit,
    const Value refUnit,
    const Value offset) noexcept : 
        _massUnit(massUnit),
        _refUnit(refUnit),
        _offset(offset) {
}

void AbstractScale::setUnit(const Mass::Unit unit) noexcept {
    this->_massUnit = unit;
}

Mass::Unit AbstractScale::getUnit() const noexcept {
    return this->_massUnit;
}

Value AbstractScale::getReferenceUnit() const noexcept {
    return this->_refUnit;
}

void AbstractScale::setReferenceUnit(const Value refUnit) {

    if(refUnit == 0) {
        throw std::invalid_argument("reference unit cannot be 0");
    }

    this->_refUnit = refUnit;

}

Value AbstractScale::getOffset() const noexcept {
    return this->_offset;
}

void AbstractScale::setOffset(const Value offset) noexcept {
    this->_offset = offset;
}

double AbstractScale::normalise(const double v) const noexcept {
    return (v - this->_offset) / this->_refUnit;
}

double AbstractScale::read(const Options o) {

    std::vector<Value> vals;

    switch(o.stratType) {
        case StrategyType::Samples:
            vals = this->getValues(o.samples);
            break;
        case StrategyType::Time:
            vals = this->getValues(o.timeout);
            break;
        default:
            throw std::invalid_argument("unknown strategy type");
    }

    if(vals.empty()) {
        throw std::runtime_error("no samples obtained");
    }

    switch(o.readType) {
        case ReadType::Median:
            return Utility::median(&vals);
        case ReadType::Average:
            return Utility::average(&vals);
        default:
            throw std::invalid_argument("unknown read type");
    }

}

void AbstractScale::zero(const Options o) {

    const auto refBackup = this->_refUnit;
    const auto offsetBackup = this->_offset;

    try {
        this->setReferenceUnit(1);
        this->setOffset(static_cast<Value>(
            std::round(this->read(o))));
        this->setReferenceUnit(refBackup);
    }
    catch(const std::exception& ex) {
        this->setReferenceUnit(refBackup);
        this->setOffset(offsetBackup);
        throw;
    }

}

Mass AbstractScale::weight(const Options o) {
    return Mass(this->normalise(this->read(o)), this->_massUnit);
}

Mass AbstractScale::weight(const std::chrono::nanoseconds timeout) {
    return this->weight(Options(timeout));
}

Mass AbstractScale::weight(const std::size_t samples) {
    return this->weight(Options(samples));
}

};
