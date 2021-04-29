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
	const std::int32_t refUnit,
	const std::int32_t offset) :
		_refUnit(refUnit),
		_offset(offset)  {
			this->_hx = new HX711(dataPin, clockPin);
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

std::int32_t HX711::getReferenceUnit() const noexcept {
	return this->_refUnit;
}

void HX711::setReferenceUnit(const std::int32_t refUnit) {
	this->_refUnit = refUnit;
}

void HX711::setChannel(const Channel ch) noexcept {
	this->_ch = ch;
}

Channel HX711::getChannel() const noexcept {
	return this->_ch;
}

void SimpleHX711::tare() {

	const std::int32_t backup = this->_refUnit;
	this->setReferenceUnit(1);
	this->_offset = this->raw();
	this->setReferenceUnit(backup);

}

Mass SimpleHX711::weight() {
	//return Mass(this->_hx->get_weight(), this->_scaleUnit);
}

double read(const std::size_t times) {
	
}

std::int32_t SimpleHX711::raw() {
	return this->_hx->getValue(this->_ch);
} 

};
