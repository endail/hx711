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
#include <iomanip>
#include <sstream>
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
    this->_u = rhs._u;
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
    return Mass(this->_g, to);
}

Mass operator+(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._g + rhs._g,
        lhs._u
    );
}

Mass operator-(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._g - rhs._g,
        lhs._u
    );
}

Mass operator*(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._g * rhs._g,
        lhs._u
    );
}

Mass operator/(const Mass& lhs, const Mass& rhs) {
    
    if(rhs._g == 0) {
        throw std::invalid_argument("cannot divide by 0");
    }
    
    return Mass(
        lhs._g / rhs._g,
        lhs._u
    );

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

Mass& Mass::operator/=(const Mass& rhs) {

    if(rhs._g == 0) {
        throw std::invalid_argument("cannot divide by 0");
    }

    this->_g /= rhs._g;
    return *this;

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

    double n; //mass as a double converted to u
    double i; //integer (discard; don't use)
    double f; //fractional
    int d = 0; //decimals

    n = Mass::convert(this->_g, Unit::G, u);
    f = std::modf(n, &i);

    /**
     * Credit: https://www.mrexcel.com/board/threads/rounding-to-first-non-zero-decimal.433225/#post-2139493
     * Minimum usable value (passed to setprecision) is 0.
     *  
     * This had a nasty bug where the expression was setting d
     * to be approx. INT_MAX and causing a seg fault in the <<
     * ostream operator below. It is necessary to guard against
     * log10(0).
     * See: https://www.cplusplus.com/reference/cmath/log10/
     */
    if(f != 0) {
        d = static_cast<int>(1 - std::log10(std::abs(f)));
    }

    /**
     * At this point d may be 1 even if the only decimal is 0. I
     * do not know why this is, but I will leave it instead of 
     * "fixing" it with string manipulation.
     */

    ss  << std::fixed
        << std::setprecision(d)
        << std::noshowpoint
        << n
        << " "
        << Mass::_UNIT_NAMES[static_cast<std::size_t>(u)];
    
    return ss.str();

}

std::ostream& operator<<(std::ostream& os, const Mass& m) noexcept {
    os << m.toString();
    return os;
}

double Mass::convert(
    const double amount,
    const Unit from,
    const Unit to) noexcept {

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