# HX711 C++ Library
- Port of https://github.com/tatobari/hx711py
- Use with Raspberry Pi

## Build and Install
```shell
pi@raspberrypi~ $ git clone https://github.com/endail/hx711
pi@raspberrypi~ $ cd hx711
pi@raspberrypi~/hx711 $ make && sudo make install
```

## Calibrate
`make` will create the executable `bin/hx711calibration` in the project directory. You can use this to calibrate your load cell and HX711 module. Run it as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 modules' clock interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

Example using WiringPi pin 8 for data and pin 9 for clock.
```shell
pi@raspberrypi~/hx711 $ bin/hx711calibration 8 9
```


## Test
`make` will create the executable `bin/test` in the project directory. You can use this to test your load cell and HX711 module. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 modules' clock interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **reference unit**: load cell's calibration factor. See above.

Example using WiringPi pin 8 for data, pin 9 for clock, and -7050 as the reference unit.
```shell
pi@raspberrypi~/hx711 $ bin/test 8 9 -7050
```


## Use
After writing your own code (eg. main.cpp), compile with the HX711 library as follows:
```shell
g++ -Wall -o prog main.cpp -l wiringPi -l hx711
```