#include <cstdlib>
#include <iostream>
#include <string>
#include <thread> // Include for sleep functionality
#include "../include/common.h"

int main(int argc, char** argv) {

    using namespace std;
    using namespace HX711;

    const char* const err = "Usage: [DATA PIN] [CLOCK PIN] [REFERENCE UNIT] [OFFSET]";

    if(argc != 5) {
        cerr << err << endl;
        return EXIT_FAILURE;
    }

    const int dataPin = stoi(argv[1]);
    const int clockPin = stoi(argv[2]);
    const int refUnit = stoi(argv[3]);
    const int offset = stoi(argv[4]);

    SimpleHX711 hx(dataPin, clockPin, refUnit, offset);

    while (true) { // Change loop to run indefinitely

        const Mass m = hx.weight(3); // Read weight

        cout << "Weight: " << m.toString(Mass::Unit::G) << " grams" << endl; // Print weight in grams

        this_thread::sleep_for(chrono::seconds(10)); // Sleep for 10 seconds
    }

    return EXIT_SUCCESS;
}