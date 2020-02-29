
#include <iostream>
#include "../include/HX711.h"
#include <unistd.h>
#include <iomanip>
#include <math.h>

int main() {

    std::cout << "Setting up..." << std::endl;

    HX711::HX711 hx(8, 9);

    std::cout << "Set up hx711" << std::endl;

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