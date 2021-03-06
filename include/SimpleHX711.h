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
#include <vector>
#include "HX711.h"
#include "Mass.h"

namespace HX711 {

/**
 * Whether SimpleHX711 will use values based on median 
 * or average values read from the sensor
 */
enum class ReadType {
    Median = 0,
    Average,
};

class SimpleHX711 {
    
protected:
    HX711* _hx;
    Mass::Unit _scaleUnit;
    Channel _ch;
    HX_VALUE _refUnit;
    HX_VALUE _offset;

    static double _median(const std::vector<HX_VALUE>* vals);
    static double _average(const std::vector<HX_VALUE>* vals);

    //prohibit copying and assignment
    SimpleHX711(const SimpleHX711& s2) noexcept;
    SimpleHX711& operator=(const SimpleHX711& rhs) noexcept;


public:

    SimpleHX711(
        const int dataPin,
        const int clockPin,
        const HX_VALUE refUnit = 1,
        const HX_VALUE offset = 0);

    ~SimpleHX711();

    void setUnit(const Mass::Unit unit) noexcept;
    Mass::Unit getUnit() const noexcept;

    HX_VALUE getReferenceUnit() const noexcept;
    void setReferenceUnit(const HX_VALUE refUnit);

    HX_VALUE getOffset() const noexcept;
    void setOffset(const HX_VALUE offset) noexcept;

    void setChannel(const Channel ch) noexcept;
    Channel getChannel() const noexcept;

    HX711* getBase() noexcept;

    /**
     * Returns a vector of values from the sensor. This method is useful
     * if you do not want to use the predefined average or median methods.
     * Note that the values in the vector are not adjusted as per the
     * reference unit or offset.
     * @param  {std::size_t} samples    : 
     * @return {std::vector<HX_VALUE>}  : 
     */
    std::vector<HX_VALUE> readValues(const std::size_t samples = 3);

    void tare(const ReadType r = ReadType::Median, const size_t samples = 3);
    Mass weight(const ReadType r = ReadType::Median, const size_t samples = 3);
    double read(const ReadType r = ReadType::Median, const size_t samples = 3);

};
};
#endif