
#include <iostream>
#include "../include/HX711.h"
#include <unistd.h>
#include <iomanip>
#include <math.h>


//  wiringPi numbered pins
//  ./test [DATA-PIN] [CLOCK-PIN]
//
int main(int argc, char** argv) {

    int dataPin = std::stoi(argv[1]);
    int clockPin = std::stoi(argv[2]);

    HX711::HX711 hx(dataPin, clockPin);

    hx.set_reference_unit(-429100);
    hx.tare();
    hx.reset();

    double weight;

    while(true) {

        weight = round(hx.get_weight(1) * 1000);

        if(weight == -0) {
            weight = 0;
        }

        std::cout
            << std::fixed
            << std::setprecision(0)
            << weight
            << " grams"
            << std::endl;

        sleep(1);
    }

    return 0;

}