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
#include <string>
#include <iomanip>
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include "../include/common.h"

int main(int argc, char** argv) {

    using namespace std;
    using namespace std::chrono;
    using namespace HX711;

    const char* const err = "Usage: [DATA PIN] [CLOCK PIN]";

    if(argc != 3) {
        cout << err << endl;
        return EXIT_FAILURE;
    }

    const int dataPin = stoi(argv[1]);
    const int clockPin = stoi(argv[2]);

    struct sched_param schParams = {
        sched_get_priority_max(SCHED_FIFO)
    };

    ::pthread_setschedparam(
        pthread_self(),
        SCHED_FIFO,
        &schParams);

    




    return EXIT_SUCCESS;

}