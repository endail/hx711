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

#include "../include/common.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

using namespace HX711;
using namespace std;

std::size_t samples;
std::string unit;
double knownWeight;
Value zeroValue;
SimpleHX711* hx;

void setupHx(const int dataPin, const int clockPin) {
    hx = new SimpleHX711(dataPin, clockPin, 1, 0);
    this_thread::sleep_for(chrono::seconds(1));
}

int main(int argc, char** argv) {

    if(argc != 3) {
        cout << "Usage: hx711calibration [data pin] [clock pin]" << endl;
        return EXIT_FAILURE;
    }

    try {
        setupHx(stoi(argv[1]), stoi(argv[2]));
    }
    catch(HX711::TimeoutException& e) {
        cout << "Failed to connect to HX711 module" << endl;
        return EXIT_FAILURE;
    }

    //clear screen
    cout    << "\x1B[2J\x1B[H"

    //splash
            << "========================================" << endl
            << "HX711 Calibration" << endl
            << "========================================"
            << endl << endl
            << "Find an object you know the weight of. If you can't find anything, "
            << "try searching Google for your phone's specifications to find its weight. "
            << "You can then use your phone to calibrate your scale."
            << endl << endl
    
    //unit prompt
            << "1. Enter the unit you want to measure the object in (eg. g, kg, lb, oz): ";
    cin >> unit;
    cin.ignore();

    //known weight prompt
    cout    << endl
            << "2. Enter the weight of the object in the unit you chose (eg. "
            << "if you chose 'g', enter the weight of the object in grams): ";
    cin >> knownWeight;
    cin.ignore();

    //samples prompt
    cout    << endl
            << "3. Enter the number of samples to take from the HX711 module (eg. 15): ";
    cin >> samples;
    cin.ignore();

    //tare prompt
    cout    << endl << "4. Remove all objects from the scale and then press enter.";
    cin.ignore();
    cout    << endl << "Working..." << flush;

    zeroValue = hx->read(Options(samples));

    //weigh prompt
    cout    << endl << endl << "5. Place object on the scale and then press enter.";
    cin.ignore();
    cout    << endl << "Working..." << flush;

    const double raw = hx->read(Options(samples));
    const double refUnitFloat = (raw - zeroValue) / knownWeight;
    const Value refUnit = static_cast<Value>(round(refUnitFloat));
    delete hx;

    cout    << endl << endl
            << "Known weight (your object): " << knownWeight << unit << endl 
            << "Raw value over " << samples << " samples: " << raw << endl
            << endl
            << "-> REFERENCE UNIT: " << refUnit << endl
            << "-> ZERO VALUE: " << zeroValue << endl
            << endl
            << "Use the reference unit value above to set the HX711 module's "
            << "reference unit. ie. using hx.setReferenceUnit()."
            << endl
            << "Use the zero value above to set the zero value of the scale "
            << "using hx.setOffset(). You won't need to tare the scale if you use "
            << "this."
            << endl << endl;

    return EXIT_SUCCESS;

}
