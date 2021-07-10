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
    :   _ug(Mass::convert(amount, u, Unit::UG)),
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

double Mass::getValue(Unit u) const noexcept {
    return Mass::convert(this->_ug, Unit::UG, u);
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
    
    std::stringstream ss;

    double n; //mass as a double converted to u
    double i; //integer (discard; don't use)
    double f; //fractional
    int d = 0; //decimals

    n = Mass::convert(this->_ug, Unit::UG, u);
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
     * do not know why this is.
     */

    ss  << std::fixed
        << std::setprecision(d)
        << std::noshowpoint
        << n
        << " "
        << _NAMES.at(u);
    
    return ss.str();

}

std::ostream& operator<<(std::ostream& os, const Mass& m) noexcept {
    os << m.toString();
    return os;
}

double Mass::convert(
    const double& amount,
    const Unit from,
    const Unit to) noexcept {

        if(to == Unit::UG) {
            return amount * _RATIOS.at(from);
        }
        
        if(from == Unit::UG) {
            return amount / _RATIOS.at(to);
        }

        return Mass::convert(amount, to, Unit::UG);

}

const std::unordered_map<const Mass::Unit, const double> Mass::_RATIOS({
    { Unit::UG, 1.0 },
    { Unit::MG, 1000.0 },
    { Unit::G,  1000000.0 },
    { Unit::KG, 1000000000.0 },
    { Unit::TON, 1000000000000.0 },
    { Unit::IMP_TON, 1016046908800.0 },
    { Unit::US_TON, 907184740000.0 },
    { Unit::ST, 6350293180.0 },
    { Unit::LB, 453592370.0 },
    { Unit::OZ, 28349523.125 }
});

const std::unordered_map<const Mass::Unit, const char* const> Mass::_NAMES({
    { Unit::UG, "μg" },
    { Unit::MG, "mg" },
    { Unit::G,  "mg" },
    { Unit::KG, "kg" },
    { Unit::TON, "ton" },
    { Unit::IMP_TON, "ton (IMP)" },
    { Unit::US_TON, "ton (US)" },
    { Unit::ST, "st" },
    { Unit::LB, "lb" },
    { Unit::OZ, "oz" }
});

};