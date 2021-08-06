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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include "../include/Mass.h"

namespace HX711 {

const std::unordered_map<const Mass::Unit, const double> Mass::_RATIOS({
    { Unit::UG,         1.0 },
    { Unit::MG,         1000.0 },
    { Unit::G,          1000000.0 },
    { Unit::KG,         1000000000.0 },
    { Unit::TON,        1000000000000.0 },
    { Unit::IMP_TON,    1016046908800.0 },
    { Unit::US_TON,     907184740000.0 },
    { Unit::ST,         6350293180.0 },
    { Unit::LB,         453592370.0 },
    { Unit::OZ,         28349523.125 }
});

const std::unordered_map<const Mass::Unit, const char* const> Mass::_UNIT_NAMES({
    { Unit::UG,         "μg" },
    { Unit::MG,         "mg" },
    { Unit::G,          "g" },
    { Unit::KG,         "kg" },
    { Unit::TON,        "ton" },
    { Unit::IMP_TON,    "ton (IMP)" },
    { Unit::US_TON,     "ton (US)" },
    { Unit::ST,         "st" },
    { Unit::LB,         "lb" },
    { Unit::OZ,         "oz" }
});

Mass::Mass(const double amount, const Unit u) noexcept
    :   _ug(convert(amount, u, Unit::UG)),
        _u(u) {
}

Mass::Mass(const Mass& m2) noexcept 
    :   _ug(m2._ug),
        _u(m2._u) {
}

Mass& Mass::operator=(const Mass& rhs) noexcept {
    this->_ug = rhs._ug;
    this->_u = rhs._u;
    return *this;
}

Mass::operator double() const noexcept {
    return this->getValue(this->_u);
}

double Mass::getValue(Unit u) const noexcept {
    return convert(this->_ug, Unit::UG, u);
}

Mass::Unit Mass::getUnit() const noexcept {
    return this->_u;
}

void Mass::setUnit(const Unit u) noexcept {
    this->_u = u;
}

Mass Mass::convertTo(const Unit to) const noexcept {
    return Mass(this->_ug, to);
}

Mass operator+(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._ug + rhs._ug,
        lhs._u
    );
}

Mass operator-(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._ug - rhs._ug,
        lhs._u
    );
}

Mass operator*(const Mass& lhs, const Mass& rhs) noexcept {
    return Mass(
        lhs._ug * rhs._ug,
        lhs._u
    );
}

Mass operator/(const Mass& lhs, const Mass& rhs) {
    
    if(rhs._ug == 0) {
        throw std::invalid_argument("cannot divide by 0");
    }
    
    return Mass(
        lhs._ug / rhs._ug,
        lhs._u
    );

}

Mass& Mass::operator+=(const Mass& rhs) noexcept {
    this->_ug += rhs._ug;
    return *this;
}

Mass& Mass::operator-=(const Mass& rhs) noexcept {
    this->_ug -= rhs._ug;
    return *this;
}

Mass& Mass::operator*=(const Mass& rhs) noexcept {
    this->_ug *= rhs._ug;
    return *this;
}

Mass& Mass::operator/=(const Mass& rhs) {

    if(rhs._ug == 0) {
        throw std::invalid_argument("cannot divide by 0");
    }

    this->_ug /= rhs._ug;
    return *this;

}

bool operator==(const Mass& lhs, const Mass& rhs) noexcept {
    return lhs._ug == rhs._ug;
}

bool operator!=(const Mass& lhs, const Mass& rhs) noexcept {
    return !operator==(lhs, rhs);
}

bool operator<(const Mass& lhs, const Mass& rhs) noexcept {
    return lhs._ug < rhs._ug;
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
    
    //std::stringstream ss;
    
    //double n; //mass as a double converted to u
    //double i; //integer
    //double f; //fractional
    //int d = 0; //decimals

    const double n = Mass::convert(this->_ug, Unit::UG, u);
    double i;
    const double f = std::modf(n, &i);
    int d = 0;

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
     * do not know why this is.
     */

    //A bit arbitrary and magic number-y, but sufficient to hold a float
    //and unit name
    const std::size_t len = 128;
    char buff[len];

    /**
     * TODO: is snprintf faster than sstream?
     */
    ::snprintf(
        buff,
        len,
        "%01.*f %s",
        d,
        n,
        _UNIT_NAMES.at(u));

    //std::string will automatically limit chars to first \0
    return std::string(buff);

/*
    ss  << std::fixed
        << std::setprecision(d)
        << std::noshowpoint
        << n
        << ' '
        << _UNIT_NAMES.at(u);
    
    return ss.str();
*/

}

std::ostream& operator<<(std::ostream& os, const Mass& m) noexcept {
    os << m.toString();
    return os;
}

double Mass::convert(
    const double amount,
    const Unit from,
    const Unit to) noexcept {

        if(from == to) {
            return amount;
        }

        if(to == Unit::UG) {
            return amount * _RATIOS.at(from);
        }
        
        if(from == Unit::UG) {
            return amount / _RATIOS.at(to);
        }

        return convert(amount, to, Unit::UG);

}

};