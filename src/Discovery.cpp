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
#include <algorithm>
#include <thread>
#include <iomanip>
#include <chrono>
#include "../include/HX711.h"
#include <gsl/gsl_statistics.h>

int main(int argc, char** argv) {

    using namespace std;
    using namespace std::chrono;
    using namespace HX711;

    const char* const err = "Usage: [DATA PIN] [CLOCK PIN] [SAMPLES]";

    if(argc != 4) {
        cout << err << endl;
        return EXIT_FAILURE;
    }

    const int dataPin = stoi(argv[1]);
    const int clockPin = stoi(argv[2]);
    const int samples = stoi(argv[3]);

    Discovery dx(dataPin, clockPin);

    std::vector<double> vals;

    for(auto ns : dx.getTimeToReady(samples)) {
        vals.push_back(static_cast<double>(duration_cast<microseconds>(ns).count()));
    }

    std::sort(vals.begin(), vals.end());

    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(0);

    cout    << "Min: " << setw(20) << right << gsl_stats_min(vals.data(), 1, vals.size()) << endl
            << "Max: " << setw(20) << right << gsl_stats_max(vals.data(), 1, vals.size()) << endl
            << "Med: " << setw(20) << right << gsl_stats_median_from_sorted_data(vals.data(), 1, vals.size()) << endl
            << endl
            ;

    return EXIT_SUCCESS;

}