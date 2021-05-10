# Raspberry Pi HX711 C++ Library

[![Build Status](https://github.com/endail/hx711/actions/workflows/buildcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/buildcheck.yml)
[![cppcheck](https://github.com/endail/hx711/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/cppcheck.yml)
[![CodeQL](https://github.com/endail/hx711/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/codeql-analysis.yml)

- Use with Raspberry Pi

## Example

```cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <hx711/SimpleHX711.h>

int main() {

  using namespace std;
  using namespace HX711;

  wiringPiSetup();

  const int dataPin = 8;
  const int clockPin = 9;
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

```text
393543766.6 μg 393543.8 mg 393.5 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393283819.6 μg 393283.8 mg 393.3 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393413793.1 μg 393413.8 mg 393.4 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393501326.3 μg 393501.3 mg 393.5 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393432360.7 μg 393432.4 mg 393.4 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393458885.9 μg 393458.9 mg 393.5 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393432360.7 μg 393432.4 mg 393.4 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393151193.6 μg 393151.2 mg 393.2 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393302387.3 μg 393302.4 mg 393.3 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
393344827.6 μg 393344.8 mg 393.3 g 0.4 kg 0.0004 ton 0.0004 ton (imp) 0.0004 ton (US) 0.06 st 0.7 lb 13.9 oz
```

## Build and Install

```shell
pi@raspberrypi~ $ git clone https://github.com/endail/hx711
pi@raspberrypi~ $ cd hx711
pi@raspberrypi~/hx711 $ make && sudo make install
```

## Calibrate

`make` will create the executable `bin/hx711calibration` in the project directory. You can use this to calibrate your load cell and HX711 module. Run it as follows and follow the prompts:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 module's clock interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

Example using WiringPi pin 8 for data and pin 9 for clock.

```shell
pi@raspberrypi~/hx711 $ bin/hx711calibration 8 9
```

## Test

`make` will create the executable `bin/simplehx711test` in the project directory. You can use this to test your load cell and HX711 module. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 modules' clock interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **reference unit**: load cell's reference unit. Find this value with the calibration program above, otherwise set it to 1.

- **offset**: load cell's offset from zero. Find this value with the calibration program above, otherwise set it to 0.

Example using WiringPi pin 8 for data, pin 9 for clock, -377 as the reference unit, and -363712 as the offset:

```shell
pi@raspberrypi~/hx711 $ bin/simplehx711test 8 9 -377 -363712
```

## Use

After writing your own code (eg. main.cpp), compile with the HX711 library as follows:

```shell
g++ -Wall -o prog main.cpp -lwiringPi -lhx711
```

Make sure to setup wiringPi with `wiringPiSetup()` ([or equivalent](http://wiringpi.com/reference/setup/)) prior to creating the SimpleHX711 object. See the [test code](https://github.com/endail/hx711/blob/master/src/SimpleHX711Test.cpp#L48) as an example.
