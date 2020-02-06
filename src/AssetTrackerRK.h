#ifndef __ASSETTRACKERRK_H
#define __ASSETTRACKERRK_H

#include "Particle.h"

#include "LIS3DH.h"
#include "TinyGPS++.h"
#include "LegacyAdapter.h"

/**
 * @brief Compatible replacement for the official Particle AssetTracker/Electron library.
 *
 * This is an almost drop-in replacement based on my LIS3DH driver and TinyGPS++.
 *
 * If I were creating a new project, I'd write directly to the LIS3DH and TinyGPS++
 * interfaces instead of using this wrapper, but it's up to you.
 *
 * Note that many of the backward compatibility methods are in the LegacyAdapter class
 * so make sure you check there as well.
 */
class AssetTracker : public LegacyAdapter {

public:
	/**
	 * @brief Construct an AssetTracker object. Normally this is a global variable.
	 */
	AssetTracker();

	/**
	 * @brief Destructor
	 */
	virtual ~AssetTracker();

	/**
	 * @brief Initialize the LIS3DH accelerometer. Optional.
	 */
	void begin(void);

	/**
	 * @brief Updates the GPS. Must be called from loop() as often as possible, typically every loop.
	 *
	 * Only call this when not using threaded mode.
	 */
	void updateGPS(void);

	/**
	 * @brief Enable GPS threaded mode
	 *
	 * In threaded mode, the serial port is read from a separate thread so you'll be less likely to
	 * lose data if loop() is blocked for any reason
	 */
	void startThreadedMode();

	/**
	 * @brief Turns the GPS on.
	 *
	 * By default, it's off on reset. Call gpsOn() to turn it on.
	 */
	void gpsOn(void);

	/**
	 * @brief Turns the GPS off.
	 */
	void gpsOff(void);

	/**
	 * @brief Read the accelerometer X value (signed)
	 */
	int readX(void);

	/**
	 * @brief Read the accelerometer Y value (signed)
	 */
	int readY(void);

	/**
	 * @brief Read the accelerometer Z value (signed)
	 */
	int readZ(void);

	/**
	 * @brief Read the magnitude of the acceleration vector (positive value)
	 *
	 * sqrt((sample.x*sample.x)+(sample.y*sample.y)+(sample.z*sample.z));
	 */
	int readXYZmagnitude(void);


	/**
	 * @brief Sends a u-blox GPS command.
	 *
	 * @param cmd The pointer to a uint8_t array of bytes to send
	 *
	 * @param len The length of the command.
	 */
	void sendCommand(const uint8_t *cmd, size_t len);


	/**
	 * @brief Select the internal antenna
	 *
	 * On the AssetTracker v2 only (or other u-blox GPS), sets the antenna to internal. Internal is the default
	 * and is reset every time the GPS is powered up.
	 *
	 * On the AssetTracker v1 (or other PA6H GPS), the antenna is auto-switching and this call is ignored.
	 */
	bool antennaInternal();


	/**
	 * @brief Select the external antenna
	 *
	 * On the AssetTracker v2 only (or other u-blox GPS), sets the antenna to external. Internal is the default
	 * and is reset every time the GPS is powered up.
	 *
	 * On the AssetTracker v1 (or other PA6H GPS), the antenna is auto-switching and this call is ignored.
	 */
	bool antennaExternal();

	/**
	 * @brief Not supported. The data is not available from TinyGPS++
	 *
	 * An empty string is always returned.
	 */
	char *preNMEA(void);

	/**
	 * @brief Calls the LIS3DH setupLowPowerWakeMode
	 *
	 * You should call this instead of begin().
	 */
	bool setupLowPowerWakeMode(uint8_t movementThreshold = 16);

	/**
	 * @brief Clears the accelerometer interrupt if latching interrupts are enabled
	 */
	uint8_t clearAccelInterrupt();

	/**
	 * @brief Calibrate the filter used to cancel out gravity
	 *
	 * @param stationaryTime Amount of time (in milliseconds) the device needs to be stationary
	 *
	 * @param maxWaitTime Amount of time to wait (in milliseconds). 0 = wait forever
	 *
	 * @return true if the filter is calibrated or false if still moving and the wait time was
	 * exceeded
	 *
	 * In order to cancel out the effects of gravity on the accelerometer, we need to wait until
	 * it's not moving. Then we assume the steady state gravity is the baseline. This is necessary
	 * to support having the GPS positioned in any direction other than completely flat
	 * (all gravity in the Z direction).
	 *
	 */
	bool calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime = 0);


	/**
	 * @brief Override the default serial port used to connect to the GPS. Default is Serial1.
	 */
	AssetTracker &withSerialPort(USARTSerial &port);

	/**
	 * @brief Use I2C (DDC) instead of serial.
	 *
	 * @param wire The I2C interface to use. This is optional, and if not set, Wire (the standard I2C interface) is used.
	 * On some devices (Electron, Argon, and Xenon), there is an optional Wire1.
	 *
	 * @param addr The I2C address to use. This is optional, and the default is 0x42.
	 * The address can be reprogrammed in software on the u-blox GPS, but 0x42 is the default.
	 */
	AssetTracker &withI2C(TwoWire &wire = Wire, uint8_t addr = 0x42);

	/**
	 * @brief Gets the LIS3DH accelerometer object so you can access its methods directly
	 */
	LIS3DHSPI *getLIS3DHSPI();

	/**
	 * @brief Gets the TinyGPS++ object so you can access its methods directly
	 */
	TinyGPSPlus *getTinyGPSPlus();


	/**
	 * @brief Sets the external decoder function
	 * 
	 * @param fn function set. Replaces any existing function. Set to 0 to remove.
	 * 
	 * This is used by the UbloxGPS module to hook into the GPS decoding loop. You
	 * probably won't have to use this directly. Other brands of GPS that decode
	 * things that TinyGPS++ can't might also use this to hook in.
	 */
	void setExternalDecoder(std::function<bool(char)> fn) { externalDecoder = fn; };

	/**
	 * @brief Lock the mutex. Used to prevent multiple threads from writing to the GPS at the same time
	 */
	void lock() { os_mutex_lock(mutex); };

	/**
	 * @brief Lock the mutex. Used to prevent multiple threads from writing to the GPS at the same time
	 */
	void unlock() { os_mutex_unlock(mutex); };

	/**
	 * @brief Get the singleton instance of this class
	 */
	static AssetTracker *getInstance() { return instance; }; 

private:
	uint16_t wireReadBytesAvailable();
	int wireReadBytes(uint8_t *buf, size_t len);

	void threadFunction();
	static void threadFunctionStatic(void *param);

	bool useWire = false;
	TwoWire &wire = Wire;
	uint8_t wireAddr = 0x42;
	USARTSerial &serialPort = Serial1;
	Thread *thread = NULL;
	os_mutex_t mutex = 0;
	std::function<bool(char)> externalDecoder = 0;
	static AssetTracker *instance;
};

#endif /* __ASSETTRACKERRK_H */
