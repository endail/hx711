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
#include <cstdint>

namespace HX711 {

SimpleHX711::SimpleHX711(
	const int dataPin,
	const int clockPin,
	const int32_t refUnit,
	const int32_t offset) {
		this->_hx = new HX711(dataPin, clockPin, refUnit, offset);
}

SimpleHX711::~SimpleHX711() {
	delete this->_hx;
}

Mass::Unit SimpleHX711::getUnit() const noexcept {
	return this->_scaleUnit;
}

void SimpleHX711::setUnit(const Mass::Unit unit) noexcept {
	this->_scaleUnit = unit;
}

void SimpleHX711::tare() {
	this->_hx->tare();
}

Mass SimpleHX711::weight() {
	return Mass(this->_hx->get_weight(), this->_scaleUnit);
}

double SimpleHX711::raw() {
	return this->_hx->get_value();
} 

};
