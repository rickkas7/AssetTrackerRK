# AssetTrackerRK
*Alternative library for the Particle AssetTracker/Electron based on TinyGPS++*

This is an alternative implementation of the library for the Particle Electron AssetTracker. I used my own [LIS3DH driver] (https://github.com/rickkas7/) and [TinyGPS++] (https://github.com/mikalhart/TinyGPSPlus).

I prefer the TinyGPS++ library as I find it to be more reliable, and my LIS3DH driver supports some more options including getting accelerometer data, direction sensing, and wake-on-move. 

There's also an optional AssetTracker almost-drop-in replacement. I prefer to access the LIS3DH driver and TinyGPS++ directly, because they have many more features than the original AssetTracker library. But for initial testing, the replacement shim is pretty handy. Note the `preNMEA()` method always return an empty string, because TinyGPS++ doesn't export a function to get that information. 

Offical project location:
[https://github.com/rickkas7/AssetTrackerRK] (https://github.com/rickkas7/AssetTrackerRK)

By the way, I highly recommend adding an external active GPS antenna and the CR1220 coin cell battery. With this combination, the wake-on-move and report GPS location example can get a GPS fix in 2 to 7 seconds!

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

