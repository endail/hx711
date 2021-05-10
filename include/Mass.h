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

#ifndef HX711_MASS_H_2FFE3D59_FB56_4C50_87F6_08F5AD88A303
#define HX711_MASS_H_2FFE3D59_FB56_4C50_87F6_08F5AD88A303

#include <string>
#include <ostream>

namespace HX711 {
struct Mass {
public:

enum class Unit {
	UG,
	MG,
	G,
	KG,
	TON,
	IMP_TON,
	US_TON,
	ST,
	LB,
	OZ
};

protected:
    static const double _CONVERSIONS[10];
    static const char* const _UNIT_NAMES[];

    //deal with mass internally as grams
    double _g;

    //unit the calling code has chosen to represent this Mass
    Unit _u;

public:
    Mass(const double amount = 0.0, const Unit u = Unit::G) noexcept;
    Mass(const Mass& m2) noexcept;

    Mass& operator=(const Mass& rhs) noexcept;
    Mass& operator=(const double& rhs) noexcept;

    double getValue(const Unit u = Unit::G) const noexcept;

    Unit getUnit() const noexcept;
    void setUnit(const Unit u) noexcept;

    Mass convertTo(const Unit to) const noexcept;

    friend Mass operator+(const Mass& lhs, const Mass& rhs) noexcept;
    friend Mass operator+(const double& lhs, const Mass& rhs) noexcept;
    friend Mass operator+(const Mass& lhs, const double& rhs) noexcept;

    friend Mass operator-(const Mass& lhs, const Mass& rhs) noexcept;
    friend Mass operator-(const double& lhs, const Mass& rhs) noexcept;
    friend Mass operator-(const Mass& lhs, const double& rhs) noexcept;

    friend Mass operator*(const Mass& lhs, const Mass& rhs) noexcept;
    friend Mass operator*(const double& lhs, const Mass& rhs) noexcept;
    friend Mass operator*(const Mass& lhs, const double& rhs) noexcept;

    friend Mass operator/(const Mass& lhs, const Mass& rhs);
    friend Mass operator/(const double& lhs, const Mass& rhs);
    friend Mass operator/(const Mass& lhs, const double& rhs);

    Mass& operator+=(const Mass& rhs) noexcept;
    Mass& operator+=(const double& rhs) noexcept;

    Mass& operator-=(const Mass& rhs) noexcept;
    Mass& operator-=(const double& rhs) noexcept;

    Mass& operator*=(const Mass& rhs) noexcept;
    Mass& operator*=(const double& rhs) noexcept;

    Mass& operator/=(const Mass& rhs);
    Mass& operator/=(const double& rhs);

    friend bool operator!(const Mass& m) noexcept;
    
    friend bool operator==(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator==(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator==(const Mass& lhs, const double& rhs) noexcept;

    friend bool operator!=(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator!=(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator!=(const Mass& lhs, const double& rhs) noexcept;

    friend bool operator<(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator<(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator<(const Mass& lhs, const double& rhs) noexcept;

    friend bool operator>(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator>(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator>(const Mass& lhs, const double& rhs) noexcept;

    friend bool operator<=(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator<=(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator<=(const Mass& lhs, const double& rhs) noexcept;

    friend bool operator>=(const Mass& lhs, const Mass& rhs) noexcept;
    friend bool operator>=(const double& lhs, const Mass& rhs) noexcept;
    friend bool operator>=(const Mass& lhs, const double& rhs) noexcept;

    std::string toString() const noexcept;
    std::string toString(const Unit u) const noexcept;

    friend std::ostream& operator<<(std::ostream& os, const Mass& m) noexcept;

    static double convert(
        const double amount,
        const Unit from,
        const Unit to = Unit::G) noexcept;

};
};
#endif