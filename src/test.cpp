// MIT License
//
// Copyright (c) 2020 Daniel Robertson
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

#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include "../include/HX711.h"

int main(int argc, char** argv) {

    using namespace std;

    const char* err = "Usage: [DATA PIN] [CLOCK PIN] [REFERENCE UNIT]";

    if(argc != 4) {
        cout << err << endl;
        return 1;
    }

    const uint8_t dataPin = stoi(argv[1]);
    const uint8_t clockPin = stoi(argv[2]);
    const int32_t refUnit = stoi(argv[3]);

    HX711::HX711 hx(dataPin, clockPin);

    hx.set_reference_unit(refUnit);
    hx.tare();

    while(true) {
        cout << fixed << hx.get_weight(1) << endl;
        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;

}