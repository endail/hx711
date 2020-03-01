#include <iostream>
#include <unistd.h>
#include <iomanip>
#include "../include/HX711.h"

int main(int argc, char** argv) {

    const char* err = "Usage: [DATA PIN] [CLOCK PIN] [REFERENCE UNIT]";

    if(argc != 4) {
        std::cout << err << std::endl;
        return -1;
    }

    int dataPin = std::stoi(argv[1]);
    int clockPin = std::stoi(argv[2]);
    int refUnit = std::stoi(argv[3]);

    HX711::HX711 hx(dataPin, clockPin);

    hx.set_reference_unit(refUnit);
    hx.tare();
    hx.reset();

    while(true) {
        std::cout
            << std::fixed
            << std::setprecision(15)
            << hx.get_weight(1)
            << std::endl;
        sleep(1);
    }

    return 0;

}