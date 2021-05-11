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

#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <wiringPi.h>
#include "../include/SimpleHX711.h"

int main(int argc, char** argv) {

    using namespace std;
    using namespace HX711;

    const char* const err = "Usage: [DATA PIN] [CLOCK PIN] [REFERENCE UNIT] [OFFSET]";

    if(argc != 5) {
        cout << err << endl;
        return EXIT_FAILURE;
    }

    const int dataPin = stoi(argv[1]);
    const int clockPin = stoi(argv[2]);
    const int refUnit = stoi(argv[3]);
    const int offset = stoi(argv[4]);

    wiringPiSetup();

    SimpleHX711 hx(dataPin, clockPin, refUnit, offset);

    //set the scale to output weights in grams
    hx.setUnit(Mass::Unit::G);
    
    while(true) {

        Mass m = hx.weight();

        cout    << m.getValue() << " "
                << m.toString(Mass::Unit::UG) << " "
                << m.toString(Mass::Unit::MG) << " "
                << m.toString(Mass::Unit::G) << " "
                << m.toString(Mass::Unit::KG) << " "
                << m.toString(Mass::Unit::TON) << " "
                << m.toString(Mass::Unit::IMP_TON) << " "
                << m.toString(Mass::Unit::US_TON) << " "
                << m.toString(Mass::Unit::ST) << " "
                << m.toString(Mass::Unit::LB) << " "
                << m.toString(Mass::Unit::OZ) << endl;

        this_thread::sleep_for(chrono::seconds(1));

    }

    return EXIT_SUCCESS;

}