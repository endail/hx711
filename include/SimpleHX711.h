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

#ifndef HX711_SIMPLEHX711_H_F776CAA5_D3AE_46D8_BD65_F4B3CD8E1DBA
#define HX711_SIMPLEHX711_H_F776CAA5_D3AE_46D8_BD65_F4B3CD8E1DBA

#include <cstdint>
#include "HX711.h"
#include "Mass.h"

namespace HX711 {
class SimpleHX711 {

protected:
	Hx711::HX711* _hx = nullptr;
	Mass::Unit _scaleUnit = Mass::Unit::G;

public:
	
	SimpleHX711(
		const int dataPin,
		const int clockPin,
		const int32_t refUnit = 1,
		const int32_t offset = 0) {
			this->_hx = new HX711(dataPin, clockPin, refUnit, offset);
	}

	~SimpleHX711() {
		delete this->_hx;
	}

	void setUnit(const Mass::Unit unit) noexcept {
		this->_scaleUnit = unit;
	}

	void tare() {
		this->_hx->tare();
	}

	Mass weight() {
		return Mass(this->_hx->get_weight(), this->_scaleUnit);
	}

	double raw() {
		return this->_hx->get_value();
	} 

};
};
#endif