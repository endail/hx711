# Raspberry Pi HX711 C++ Library

[![Build Status](https://github.com/endail/hx711/actions/workflows/buildcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/buildcheck.yml)
[![cppcheck](https://github.com/endail/hx711/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/cppcheck.yml)
[![CodeQL](https://github.com/endail/hx711/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/codeql-analysis.yml)

- Use with Raspberry Pi
- Requires [lgpio](http://abyz.me.uk/lg/index.html)

## Example

```cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <hx711/HX711.h>

int main() {

  using namespace std;
  using namespace HX711;

  const int dataPin = 2;
  const int clockPin = 3;
  const int referenceUnit = -377;
  const int offset = -363712;

  SimpleHX711 hx(dataPin, clockPin, referenceUnit, offset);

  //make the hx output weights in kilograms
  hx.setUnit(Mass::Unit::KG);

  while(true) {
    cout << hx.weight() << endl; //prints eg. "36.08 kg"
    this_thread::sleep_for(chrono::seconds(1));
  }

  return 0;

}
```

## Sample Output from Test Code

See: [`src/SimpleHX711Test.cpp`](https://github.com/endail/hx711/blob/master/src/SimpleHX711Test.cpp)

![hx711.gif](hx711.gif)

## Build and Install

```shell
pi@raspberrypi~ $ git clone https://github.com/endail/hx711
pi@raspberrypi~ $ cd hx711
pi@raspberrypi~/hx711 $ make && sudo make install
```

## Calibrate

`make` will create the executable `bin/hx711calibration` in the project directory. You can use this to calibrate your load cell and HX711 module. Run it as follows and follow the prompts:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [GPIO](https://pinout.xyz/) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 module's clock interface. Use [GPIO](https://pinout.xyz/) pin numbering.

Example using GPIO pin 2 for data and pin 3 for clock.

```shell
pi@raspberrypi~/hx711 $ bin/hx711calibration 2 3
```

## Test

`make` will create the executable `bin/simplehx711test` in the project directory. You can use this to test your load cell and HX711 module. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [GPIO](https://pinout.xyz/) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 modules' clock interface. Use [GPIO](https://pinout.xyz/) pin numbering.

- **reference unit**: load cell's reference unit. Find this value with the calibration program above, otherwise set it to 1.

- **offset**: load cell's offset from zero. Find this value with the calibration program above, otherwise set it to 0.

Example using GPIO pin 2 for data, pin 3 for clock, -377 as the reference unit, and -363712 as the offset:

```shell
pi@raspberrypi~/hx711 $ bin/simplehx711test 2 3 -377 -363712
```

## Use

After writing your own code (eg. main.cpp), compile with the HX711 library as follows:

```shell
g++ -Wall -o prog main.cpp -lhx711 -llgpio
```
