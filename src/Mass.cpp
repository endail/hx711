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

#include "../include/Mass.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

namespace HX711 {

Mass::Mass(const double amount, const Unit u) noexcept
    :   _g(Mass::convert(amount, u, Unit::G)),
        _u(u) {
}

Mass::Mass(const Mass& m2) noexcept 
    :   _g(m2._g),
        _u(m2._u) {
}

Mass& Mass::operator=(const Mass& rhs) noexcept {
    this->_g = rhs._g;
    //don't copy the unit
    return *this;
}

Mass& Mass::operator=(const double& rhs) noexcept {
    this->_g = rhs;
    return *this;
}

double Mass::getValue(Unit u) const noexcept {
    return Mass::convert(this->_g, Unit::G, u);
}

Mass::Unit Mass::getUnit() const noexcept {
    return this->_u;
}

void Mass::setUnit(const Unit u) noexcept {
    this->_u = u;
}

Mass Mass::convertTo(const Unit to) const noexcept {
    Mass m = *this;
    m._u = to;
    return m;
}

Mass operator+(const Mass& lhs, const Mass& rhs) noexcept {
    Mass m;
    m._g = lhs._g + rhs._g;
    m._u = lhs._u;
    return m;
}

Mass operator-(const Mass& lhs, const Mass& rhs) noexcept {
    Mass m;
    m._g = lhs._g - rhs._g;
    m._u = lhs._u;
    return m;
}

Mass operator*(const Mass& lhs, const Mass& rhs) noexcept {
    Mass m;
    m._g = lhs._g * rhs._g;
    m._u = lhs._u;
    return m;
}

Mass operator/(const Mass& lhs, const Mass& rhs) noexcept {
    
    if(rhs._g == 0) {
        throw std::invalid_argument("cannot divide by 0");
    }
    
    Mass m;
    
    m._g = lhs._g / rhs._g;
    m._u = lhs._u;
    
    return m;

}

Mass& Mass::operator+=(const Mass& rhs) noexcept {
    this->_g += rhs._g;
    return *this;
}

Mass& Mass::operator-=(const Mass& rhs) noexcept {
    this->_g -= rhs._g;
    return *this;
}

Mass& Mass::operator*=(const Mass& rhs) noexcept {
    this->_g *= rhs._g;
    return *this;
}

Mass& Mass::operator/=(const Mass& rhs) noexcept {

    if(rhs._g == 0) {
        throw std::invalid_argument("cannot divide by 0");
        
    }

    this->_g /= rhs._g;
    return *this;

}

bool operator!(const Mass& m) noexcept {
    return m._g == 0;
}

bool operator==(const Mass& lhs, const Mass& rhs) noexcept {
    return lhs._g == rhs._g;
}

bool operator!=(const Mass& lhs, const Mass& rhs) noexcept {
    return !operator==(lhs, rhs);
}

bool operator<(const Mass& lhs, const Mass& rhs) noexcept {
    return lhs._g < rhs._g;
}

bool operator>(const Mass& lhs, const Mass& rhs) noexcept {
    return operator<(rhs, lhs);
}

bool operator<=(const Mass& lhs, const Mass& rhs) noexcept {
    return !operator>(lhs, rhs);
}

bool operator>=(const Mass& lhs, const Mass& rhs) noexcept {
    return !operator<(lhs, rhs);
}

std::string Mass::toString() const noexcept {
    return this->toString(this->_u);
}

std::string Mass::toString(const Unit u) const noexcept {
    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(0);
    ss << std::round(Mass::convert(this->_g, Unit::G, u));
    ss << " ";
    ss << Mass::_UNIT_NAMES[static_cast<std::size_t>(u)];
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Mass& m) noexcept {
    return os << m.toString();
}

double Mass::convert(
    const double amount,
    const Unit from,
    const Unit to) noexcept {

        //return early if 0
        if(amount == 0) {
            return 0;
        }

        if(to == Unit::G) {
            return amount * Mass::_CONVERSIONS[static_cast<std::size_t>(from)];
        }
        else if(from == Unit::G) {
            return amount / Mass::_CONVERSIONS[static_cast<std::size_t>(to)];
        }

        return Mass::convert(amount, to, Unit::G);

}

// ratios of units to g
const double Mass::_CONVERSIONS[10] = {
    1e-6,           //ug
    0.001,          //mg
    1.0,            //g
    1000.0,         //kg
    1e+6,           //ton
    1.016e+6,       //ton (IMP)
    907185,         //ton (US)
    6350.29,        //st
    535.592,        //lb
    28.3495         //oz
};

const char* const Mass::_UNIT_NAMES[] = {
    "μg",
    "mg",
    "g",
    "kg",
    "ton",
    "ton (imp)",
    "ton (US)",
    "st",
    "lb",
    "oz"
};

};