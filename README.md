# HX711 C++ Library

- port of https://github.com/tatobari/hx711py
- use with Raspberry Pi

## Build and Install
```shell
pi@raspberrypi~ $ git clone https://github.com/endail/hx711
pi@raspberrypi~ $ cd hx711
pi@raspberrypi~ $ make
pi@raspberrypi~ $ make install
pi@raspberrypi~ $ make test
```

## Test
`make test` will create an executable in the `bin` directory. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 module's data interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **clock pin**: Raspberry Pi pin which connects to the HX711 modules' clock interface. Use [WiringPi](https://pinout.xyz/pinout/wiringpi) pin numbering.

- **reference unit**: load cell's calibration factor, if you know it.

```
pi@raspberrypi~ $ sudo bin/test 8 9 -7050
```