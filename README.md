# Raspberry Pi HX711 C++ Library

[![Build on Raspberry Pi](https://github.com/endail/hx711/actions/workflows/buildcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/buildcheck.yml) [![cppcheck](https://github.com/endail/hx711/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/endail/hx711/actions/workflows/cppcheck.yml)

- Use with Raspberry Pi
- Requires [lgpio](http://abyz.me.uk/lg/index.html)
- Developed and tested with a Raspberry Pi Zero W (should work on other Pis)
- Looking for a Python alternative? [Try here](https://github.com/endail/hx711-rpi-py).

## Sample Output from Test Code

See: [src/SimpleHX711Test.cpp](src/SimpleHX711Test.cpp)

![hx711.gif](resources/hx711.gif)

The .gif above illustrates the output of the test code where I applied pressure to the load cell. The HX711 chip was operating at 80Hz. However, note from the [code](src/SimpleHX711Test.cpp) that the value being used is the median of three samples from the sensor. Also note the automatic formatting of the floating point numbers.

## Build and Install

```console
pi@raspberrypi:~ $ git clone https://github.com/endail/hx711
pi@raspberrypi:~ $ cd hx711
pi@raspberrypi:~/hx711 $ make && sudo make install
```

## Use

After writing your own code (eg. main.cpp), compile and link with the HX711 and lgpio libraries as follows:

```console
pi@raspberrypi:~ $ g++ -Wall -o prog main.cpp -lhx711 -llgpio
```

## Examples

### SimpleHX711 Example

```c++
#include <iostream>
#include <hx711/common.h>

int main() {

  using namespace HX711;

  // create a SimpleHX711 object using GPIO pin 2 as the data pin,
  // GPIO pin 3 as the clock pin, -370 as the reference unit, and
  // -367471 as the offset
  SimpleHX711 hx(2, 3, -370, -367471);

  // set the scale to output weights in ounces
  hx.setUnit(Mass::Unit::OZ);

  // constantly output weights using the median of 35 samples
  for(;;) std::cout << hx.weight(35) << std::endl; //eg. 1.08 oz

  return 0;

}
```

### AdvancedHX711 Example

```c++
#include <chrono>
#include <iostream>
#include <hx711/common.h>

int main() {

  using namespace HX711;
  using std::chrono::seconds;

  // create an AdvancedHX711 object using GPIO pin 2 as the data pin,
  // GPIO pin 3 as the clock pin, -370 as the reference unit, -367471
  // as the offset, and indicate that the chip is operating at 80Hz
  AdvancedHX711 hx(2, 3, -370, -367471, Rate::HZ_80);

  // constantly output weights using the median of all samples
  // obtained within 1 second
  for(;;) std::cout << hx.weight(seconds(1)) << std::endl; //eg. 0.03 g

  return 0;

}
```

## Calibrate

`make` will create the executable `bin/hx711calibration` in the project directory. You can use this to calibrate your load cell and HX711 chip. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 chip's data pin.

- **clock pin**: Raspberry Pi pin which connects to the HX711 chip's clock pin.

Example using GPIO pin 2 for data and GPIO pin 3 for clock.

```console
pi@raspberrypi:~/hx711 $ bin/hx711calibration 2 3
```

## Test

`make` will create the executables `bin/simplehx711test` and `bin/advancedhx711test` in the project directory. You can use these programs to test your load cell and HX711 module. Arguments are as follows:

- **data pin**: Raspberry Pi pin which connects to the HX711 chip's data pin.

- **clock pin**: Raspberry Pi pin which connects to the HX711 chip's clock pin.

- **reference unit**: load cell's reference unit. Find this value with the calibration program above, otherwise set it to 1.

- **offset**: load cell's offset from zero. Find this value with the calibration program above, otherwise set it to 0.

Example using GPIO pin 2 for data, GPIO pin 3 for clock, -377 as the reference unit, and -363712 as the offset:

```console
pi@raspberrypi:~/hx711 $ bin/simplehx711test 2 3 -377 -363712
```

Same example, except running as root using `sudo` with the `AdvancedHX711` to use real-time scheduling. More information about this is avaliable below.

```console
pi@raspberrypi:~/hx711 $ sudo bin/advancedhx711test 2 3 -377 -363712
```

## Documentation

### Datasheet

[Revision 2.0](resources/hx711F_EN.pdf)

### Wiring and Pins

The Sparkfun website has a [tutorial](https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide) on how to connect a HX711 breakout board to a load cell and to a microcontroller such as an Arduino. When connecting to a Raspberry Pi, the only significant difference is to connect the breakout board's `VCC` pin to a Raspberry Pi [5v pin](https://pinout.xyz/pinout/5v_power), and the `VDD` pin to a Raspberry Pi [3.3v pin](https://pinout.xyz/pinout/3v3_power). **Be very careful not to confuse the two or you could damage your Raspberry Pi**.

Unless otherwise stated, use [GPIO](https://pinout.xyz/) pin numbering. You do not need to use the dedicated Raspberry Pi [SPI](https://pinout.xyz/pinout/spi) or [I2C](https://pinout.xyz/pinout/i2c) pins. The HX711 is **not** an I2C device. Any pin capable of input and output may be used.

---

There are two relevant classes for interfacing with a HX711: `SimpleHX711` and `AdvancedHX711`.

### [SimpleHX711( int dataPin, int clockPin, Value refUnit = 1, Value offset = 0, Rate rate = Rate::HZ_10 )](include/SimpleHX711.h)

- **dataPin**: Raspberry Pi pin which connects to the HX711 chip's data pin (also referred to as DOUT).

- **clockPin**: Raspberry Pi pin which connects to the HX711 chip's clock pin (also referred to as PD_SCK).

- **refUnit**: load cell's reference unit. Find this value with the calibration program described below, otherwise set it to 1.

- **offset**: load cell's offset from zero. Find this value with the calibration program described below, otherwise set it to 0.

- **rate**: HX711 chip's data rate. Changing this does **not** alter the rate at which the HX711 chip outputs data, but it is used to determine the correct data settling time. Changing the data rate requires modification of the hardware. On [Sparkfun's HX711 breakout board](https://www.sparkfun.com/products/13879), there is a [jumper on the bottom of the board](resources/13879-SparkFun_Load_Cell_Amplifier_-_HX711-03.jpg) labelled `RATE`. By default, the jumper is closed, which sets the data rate to 10Hz. Opening the jumper (by cutting between the solder pads with a blade, desoldering, etc...) sets the data rate to 80Hz. Also see `SJ2` in the [schematic](resources/SparkFun_HX711_Load_Cell.pdf).

As the name implies, this is a simple interface to the HX711 chip. Its core operation is [busy-waiting](https://en.wikipedia.org/wiki/Busy_waiting). It will continually check whether data is ready to be obtained from the HX711 chip. This is both its advantage and disadvantage. It is as fast as possible, but uses more of the CPU's time.

---

### [AdvancedHX711( int dataPin, int clockPin, Value refUnit = 1, Value offset = 0, Rate rate = Rate::HZ_10 )](include/AdvancedHX711.h)

Arguments are identical to `SimpleHX711`.

The `AdvancedHX711` is an effort to minimise the time spent by the CPU checking whether data is ready to be obtained from the HX711 module while remaining as efficient as possible. Its core operation, in contrast to `SimpleHX711`, is through the use of a separate thread of execution to intermittently watch for and collect data when it is available.

Additionally, the thread watching for and collecting data will alter its own CPU scheduling priority accordingly if it has permission to. In practice, this means that if executed with `sudo`, the thread will run in "[real-time](https://man7.org/linux/man-pages/man7/sched.7.html)". You will note that from running `htop` simultaneously with the advancedhx711test program there is an entry for the watching thread with its priority set to RT (real-time). For example:

```console
pi@raspberrypi:~/hx711 $ sudo bin/advancedhx711test 2 3 -377 -363712
```

```console
  CPU[|||||||||||||                     30.6%]   Tasks: 34, 10 thr; 1 running
  Mem[|||||||||||||||||            39.4M/478M]   Load average: 0.77 0.53 0.27
  Swp[                              0K/100.0M]   Uptime: 1 day, 01:55:01

  PID USER      PRI  NI  VIRT   RES   SHR S CPU% MEM%   TIME+  Command
 1851 root       RT   0 30908  3012  2784 S 22.4  0.6  0:01.06 bin/advancedhx711test 2 3 -377 -363712
```

---

### [HX711](include/HX711.h)

`SimpleHX711` and `AdvancedHX711` both inherit from the `HX711` class and provides these additional functions.

- `bool isReady( )`. Returns true if the HX711 chip has data ready to be retrieved.

- `void setStrictTiming( bool strict )`. The HX711 chip has specific timing requirements which if not adhered to may lead to corrupt data. If strict timing is enabled, an `IntegrityException` will be thrown when data integrity cannot be guaranteed. However, given the unreliability of timing on a non-realtime OS (such as Raspbian on a Raspberry Pi), this in itself is unreliable and therefore disabled by default. Use at your own risk.

- `bool isStrictTiming( )`. Returns true if strict timing is used.

- `void useDelays( bool use )`. If true, _very_ short delays will be used during the period during which bits are read from the HX711 chip. These delays conform to the datasheet's specifications. On a Raspberry Pi, using delays is not likely to be useful unless, for some reason, the CPU is _too_ fast. For that reason, the default is not to use them.

- `bool isUsingDelays( )`. Returns true if delays are in use. See above.

- `void setFormat( Format bitFormat )`. Defines the format of bits when read from the HX711 chip. Either `Format::MSB` (most significant bit first - the default) or `Format::LSB` (least significant bit first).

- `Format getFormat( )`. Returns the `Format` currently being used.

- `int getDataPin( )`. Returns the GPIO pin number connected to the HX711's data pin (DOUT).

- `int getClockPin( )`. Returns the GPIO pin number connected to the HX711's clock pin (PD_SCK).

- `Channel getChannel( )`. Returns the `Channel` being used when reading from the HX711 chip.

- `Gain getGain( )`. Returns the `Gain` being used when reading from the HX711 chip.

- `void setConfig( Channel c = Channel::A, Gain g = Gain::GAIN_128 )`. Changes the channel and gain of the HX711 chip. An `std::invalid_argument` will be thrown if the given channel and gain are incompatible. Channel A may be set to a gain of 64 or 128. Channel B may only use a gain of 32. Please see the datasheet for more information.

- `Value readValue( )`. Reads a value from the HX711 chip. You should generally **not** use this method. If you do, you must check whether the HX711 has data ready to be read (see: `.isReady( )`).

- `void powerDown()`

- `void powerUp()`

---

### [AbstractScale](include/AbstractScale.h)

`SimpleHX711` and `AdvancedHX711` also both inherit from the `AbstractScale` class. This is the interface between raw data values from the HX711 chip and the functionality of a scale.

- `Mass::Unit getUnit()` and `void setUnit( Mass::Unit unit )`. Gets and sets the unit the scale will return weights in. For example, if set to `Mass::Unit::KG`, the scale will output a weight in kilograms. The default unit is grams (`Mass::Unit::G`).

- `Value getReferenceUnit()` and `void setReferenceUnit( Value refUnit )`. See calibration program.

- `Value getOffset()` and `void setOffset( Value offset )`. Offset from zero. See calibration program.

- `double normalise( double v )`. Given a raw value from HX711, returns a "normalised" value adjusted according to the scale's reference unit and offset.

- `std::vector<Value> getValues( std::size_t samples )`. Returns a vector of `samples` number of raw `Value`s from the HX711 chip. You should use this method if you want to deal with raw, numeric values which have not been adjusted for weighing functions.

- `std::vector<Value> getValues( std::chrono::nanoseconds timeout )`. Returns a vector of raw `Value`s obtained from the HX711 chip within `timeout`. You should use this method if you want to deal with raw, numeric values which have not been adjusted for weighing functions.

- `double read( Options o = Options() )`. Returns a numeric value from the scale according to the given `Options`. The returned value has **not** been adjusted with .`normalise()`. You should use this method if you want to deal with a **single** numeric value which has not been adjusted for weighing functions.

- `void zero( Options o = Options() )`. Zeros the scale.

- `Mass weight( Options o = Options() )`. Returns the current weight on the scale according to the given `Options`.

- `Mass weight( std::size_t samples )`. Returns the current weight on the scale using the median value from `samples` number of samples.

- `Mass weight( std::chrono::nanoseconds timeout )`. Returns the current weight on the scale using the median value from all samples collected within the `timeout` period.

---

### Options

You will notice in the functions above there is an `Options` parameter. This determines how data is collected and interpreted according to a `StrategyType` and `ReadType`.

- `StrategyType::Samples` instructs the scale to collect `Options.samples (std::size_t)` number of samples. This is the default.

- `StrategyType::Time` instructs the scale to collect as many samples as possible within the time period `Options.timeout (std::chrono::nanoseconds)`.

- `ReadType::Median` instructs the scale to use the median value from the collected samples. This is the default.

- `ReadType::Average` instructs the scale to use the average value from the collected samples.

---

### [Mass](include/Mass.h)

`Mass` is a self-contained class to easily convert between units of mass. A `Mass` object contains a value stored as a `double` and a `Mass::Unit` representing the unit of that value. Methods of the `Mass` class you may find particularly useful include:

- `Unit getUnit()` and `setUnit( Unit u )` to find and change the unit of mass.

- `Mass convertTo( Unit to )` to return a new `Mass` object with the given `to` unit.

- `std::string toString()` which returns a formatted string containing the value of the `Mass` object in the accompanying unit of mass, followed the unit name. eg. "1.03 kg".

- `std::string toString( Unit u )` which performs the same function as above, except according to the given `Unit` `u`.

- `std::ostream& operator<<( std::ostream& os, Mass& m )` to send the output of `toString()` to the `ostream`. For example:

```c++
Mass m(1.03, Mass::Unit::KG);
std::cout << m; //1.03 kg
```

- Unary and binary operators are also supported.

The following `Mass::Unit`s are supported:

| identifier            | description   | toString suffix |
| --------------------- | --------------| --------------- |
| `Mass::Unit::UG`      | micrograms    | μg              |
| `Mass::Unit::MG`      | milligrams    | mg              |
| `Mass::Unit::G`       | grams         | g               |
| `Mass::Unit::KG`      | kilograms     | kg              |
| `Mass::Unit::TON`     | metric tons   | ton             |
| `Mass::Unit::IMP_TON` | imperial tons | ton (IMP)       |
| `Mass::Unit::US_TON`  | US tons       | ton (US)        |
| `Mass::Unit::ST`      | stones        | st              |
| `Mass::Unit::LB`      | pounds        | lb              |
| `Mass::Unit::OZ`      | ounces        | oz              |

### Noise

It is possible that the HX711 chip will return - or the code will read - an invalid value or "noise". I have opted not to filter these values in this library and instead leave them up to the individual developer on how best to go about doing so for their individual application.

With that said, if you are looking for a simple but effective method to filter momentary noise, I highly recommend taking the median of at least three samples from the sensor. The [SimpleHX711Test.cpp code](src/SimpleHX711Test.cpp) does this and can be seen in the .gif above.

### Other Notes

- All HX711 library code exists within the `HX711` namespace

- After building and installing the library (see below), you can `#include <hx711/common.h>` to include everything

- `sudo make uninstall` from the project directory to remove the library

- If you are looking for a version of this library which uses wiringPi rather than lgpio, [v1.1 is available](https://github.com/endail/hx711/releases/tag/1.1). However, given that [wiringPi is deprecated](http://wiringpi.com/wiringpi-deprecated/), I have chosen to use lgpio going forward.

### FAQ

***"I just want to get some raw numbers from the scale".***

There are a few different methods for this.

1. `getValues( std::size_t samples )` and `getValues( std::chrono::nanoseconds timeout )` are both accessible from `SimpleHX711` and `AdvancedHX711` and return an `std::vector<Value>` containing raw, unadjusted values from the HX711 chip. You should use this.

2. `double read( Options o = Options() )` is accessible from `SimpleHX711` and `AdvancedHX711`. The difference between `.read()` and `.getValues()` is that `.read()` takes an optional `Options` argument to filter and return a **single** value. For example, by finding the average or median.

3. `HX711::readValue` is essentially what `.getValues()` uses. But calling `readValue()` does not check whether the HX711 chip is ready for a value to be read. Using this on its own will produce unreliable results.

***"What's the difference between `SimpleHX711` and `AdvancedHX711`?"***

`AdvancedHX711` uses a separate thread of execution to watch for and collect values from the HX711 chip when they are ready. It aims to be as efficient as possible. I recommend using `AdvancedHX711` when you are obtaining a large number of samples.
