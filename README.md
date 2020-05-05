# AssetTrackerRK

*Alternative library for the Particle AssetTracker/Electron based on TinyGPS++*

This is an alternative implementation of the library for the Particle Electron AssetTracker. I used my own [LIS3DH driver](https://github.com/rickkas7/) and [TinyGPS++](https://github.com/mikalhart/TinyGPSPlus).

I prefer the TinyGPS++ library as I find it to be more reliable, and my LIS3DH driver supports some more options including getting accelerometer data, direction sensing, and wake-on-move. 

In addition to the Electron-based AssetTracker v1 and v2, it can be used with many GPS boards that use either the PA6H (PMTK protocol) or u-blox line of serial-connected GPS chipsets.

There's also an optional AssetTracker almost-drop-in replacement. I prefer to access the LIS3DH driver and TinyGPS++ directly, because they have many more features than the original AssetTracker library. But for initial testing, the replacement shim is pretty handy. Note the `preNMEA()` method always return an empty string, because TinyGPS++ doesn't export a function to get that information. 

There is also support for threaded mode. In threaded mode, the GPS serial port is read from a separate thread. This eliminates the need to do it from loop, and dramatically reduces the chance of lost or corrupted GPS data caused by blocking the loop long enough to overflow the 64-byte serial buffer.

Official project location:
[https://github.com/rickkas7/AssetTrackerRK](https://github.com/rickkas7/AssetTrackerRK)

You can browse the full API documentation here:
[https://rickkas7.github.io/AssetTrackerRK](https://rickkas7.github.io/AssetTrackerRK).

Also note: If you are using this directly from the Particle Firmware Libraries manager it will automatically include the LIS3DH driver as well. If you are manually copying the source from here, you will also need:

[https://github.com/rickkas7/LIS3DH](https://github.com/rickkas7/LIS3DH)

By the way, I highly recommend adding an external active GPS antenna and the CR1220 coin cell battery with the AssetTracker V1. With this combination, the wake-on-move and report GPS location example can get a GPS fix in 2 to 7 seconds! 

The AssetTracker V2 includes a supercapacitor that eliminates the need for the coin cell as long as you normally have power available to the GPS.

## Examples

The code should be straightforward to use. The examples include:

### 1 Simple GPS

Simple example of using the GPS to report the current location to serial and Publish to the cloud.

### 2 GPS Celullar On/Off

Like the previous example, but supports turning cellular on or off. Pressing the MODE button toggles the cellular connection.

### 3 Wake On Move

Wakes the Electron when moved and publishes the current location from the GPS.

### 4 Accelerometer Samples

Prints XYZ data from the accelerometer to the debug USB serial connection.

### 5 Accelerometer Direction

The LIS3DH accelerometer has the ability to detect when it's stable in one of 6 orientations. This code prints the current orientation to the debug USB serial connection.

### 6 Backward Compatible API

Example of using the original AssetTracker API with the this library.

### 7 Data Dump Tool

This prints the raw data from the GPS, which can be useful for debugging.

### 8 Threaded Mode

This uses the threaded mode, which allows the GPS to run efficiently and without errors if you have blocking
code in your loop() function. This is recommended instead of the simple method used earlier.

### 9 I2C

Version 0.2.2 and later of the library supports a u-blox GPS (tested with a MAX-M8-Q) using I2C instead of serial. This is
particularly helpful on Gen 3 devices like the Boron, which does not have as many serial ports as the Electron.

### 10 Pass-Through

The Pass-Through firmware is used primarily so you can use the u-blox [u-center](https://www.u-blox.com/en/product/u-center) app with the AssetTracker V2. This Windows-based software allows you to decode the data from the GPS and issue commands using a graphical user interface. 

Flash the pass-through software to your AssetTracker V2. It will boot into manual mode (breathing white). Run the u-center application and select the USB port for the Electron. 

In order to remove the pass-through software you'll probably need to use DFU mode (blinking yellow) or safe mode (breathing magenta) since the cloud is deactivated by this example.

### 11 AssistNow

This example uses the u-blox AssistNow service to download almanac and ephemeris data over cellular and upload it to the modem, instead of waiting for it to arrive by satellite. This can greatly speed time to first fix on compatible GPS models. See [AssistNow](https://github.com/rickkas7/AssetTrackerRK/blob/master/AssistNow.md) for more information.

## Updates

### Updated in 0.1.1

Fixed compatibility with system firmware 0.6.1. The Arduino compatibility in that version conflicted with the Arduino compatibility built into TinyGPS++.

### Updated in 0.1.2

Added compatibility with AssetTracker v2. This is just basic compatibility with getting GPS data; more advanced 
options for controlling it will be added later.

### Updated in 0.2.0

- Implement all official AssetTracker methods
- Upgrade to LIS3DH 0.2.3
- Refactoring
- Parser Unit Tests

### Updated in 0.2.1

- Additional documentation
- Support for threaded mode

### Updated in 0.2.2

- Support for u-blox GPS connected by I2C instead of serial

### Updated in 0.3.1

- Minimum supported Device OS version is now 0.9.0. Tested mostly with 1.4.4.
- Added u-blox [AssistNow](https://github.com/rickkas7/AssetTrackerRK/blob/master/AssistNow.md) support (beta)
- Added u-center pass-through example

### Updated for 0.3.2

- Updated to LIS3DH 0.2.5 to fix a SOS+1 at startup when used with Device OS 1.5.0.

### Updated for 0.3.3

- Update to LIS3DH 0.2.6 to fix compiler error for ambiguous requestFrom with 1.5.0-rc.2.

### Updated for 0.3.4 (2020-05-04)

- Update to LIS3DH 0.2.7 to fix compiler error for undefined conversion to SpiClass with 1.5.1-rc.1. 

### Updated for 0.3.5 (2020-05-04)

- Update to LIS3DH 0.2.8 to fix compiler error for undefined conversion to SpiClass with 1.5.1-rc.1. The fix in 0.2.7 didn't work right with SPI1.


