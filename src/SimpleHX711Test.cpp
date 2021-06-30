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
#include <numeric>
#include <string>
#include "../include/HX711.h"

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

    SimpleHX711 hx(dataPin, clockPin, refUnit, offset);
    
int count = 0;

    auto samples = hx.getBase()->testTiming();
    long long unsigned int sum = 0;

    for(int i = 0; i < samples.size(); ++i) {
        sum += samples[i].getDiff().count();
    }

    double mean = sum / samples.size();

    cout << "mean: " << mean << endl;
    
    //for(auto it = samples.begin(); it != samples.end(); ++it) {
    //    cout << it->count() << endl;
    //}

    return EXIT_SUCCESS;

    while(true) {

        //use the median from 5 samples
        Mass m = hx.weight(ReadType::Median, 3);
        //count++;

        //cout << "\x1B[2J\x1B[H" << double(count % 80) << endl;
        //continue;

        cout    << "\x1B[2J\x1B[H"
                << "\t" << m.getValue() << endl
                << "\t" << m.toString(Mass::Unit::UG) << endl
                << "\t" << m.toString(Mass::Unit::MG) << endl
                << "\t" << m.toString(Mass::Unit::G) << endl
                << "\t" << m.toString(Mass::Unit::KG) << endl
                << "\t" << m.toString(Mass::Unit::TON) << endl
                << "\t" << m.toString(Mass::Unit::IMP_TON) << endl
                << "\t" << m.toString(Mass::Unit::US_TON) << endl
                << "\t" << m.toString(Mass::Unit::ST) << endl
                << "\t" << m.toString(Mass::Unit::LB) << endl
                << "\t" << m.toString(Mass::Unit::OZ) << endl
        ;

    }

    return EXIT_SUCCESS;

}