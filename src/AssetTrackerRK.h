#ifndef __ASSETTRACKERRK_H
#define __ASSETTRACKERRK_H

#include "Particle.h"

#include "LIS3DH.h"
#include "TinyGPS++.h"
#include "LegacyAdapter.h"
#include "UbloxGPS.h"

class AssetTrackerLIS3DH {
public:
	AssetTrackerLIS3DH(LIS3DH *accel);
	virtual ~AssetTrackerLIS3DH();

	void begin(void);

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


protected:
	LIS3DH *accel;
};

/**
 * @brief Base functionality for GNSS functionality using TinyGPS
 */
class AssetTrackerBase : public LegacyAdapter {
public:
	AssetTrackerBase();
	virtual ~AssetTrackerBase();

	void begin(); 

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


	void enterSleep();

	void wake();

	/**
	 * @brief Sends a u-blox GPS command.
	 *
	 * @param cmd The pointer to a uint8_t array of bytes to send
	 *
	 * @param len The length of the command.
	 */
	void sendCommand(const uint8_t *cmd, size_t len);

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
	 * @brief Set a function to be called during the threaded mode loop
	 */
	void setThreadCallback(std::function<void(void)> fn) { threadCallbacks.push_back(fn); };

	/**
	 * @brief Set a function to be called during the when a full sentence is received
	 */
	void setSentenceCallback(std::function<void(void)> fn) { sentenceCallbacks.push_back(fn); };

	/**
	 * @brief Override the default serial port used to connect to the GPS. Default is Serial1.
	 */
	AssetTrackerBase &withSerialPort(USARTSerial &port);

	/**
	 * @brief Use I2C (DDC) instead of serial.
	 *
	 * @param wire The I2C interface to use. This is optional, and if not set, Wire (the standard I2C interface) is used.
	 * On some devices (Electron, Argon, and Xenon), there is an optional Wire1.
	 *
	 * @param addr The I2C address to use. This is optional, and the default is 0x42.
	 * The address can be reprogrammed in software on the u-blox GPS, but 0x42 is the default.
	 */
	AssetTrackerBase &withI2C(TwoWire &wire = Wire, uint8_t addr = 0x42);

	AssetTrackerBase &withGNSSExtInt(pin_t extIntPin) { this->extIntPin = extIntPin; return *this; };

	/**
	 * @brief Gets the TinyGPS++ object so you can access its methods directly
	 */
	TinyGPSPlus *getTinyGPSPlus() { return &gps; };

	/**
	 * @brief Lock the mutex. Used to prevent multiple threads from writing to the GPS at the same time
	 */
	void lock();

	/**
	 * @brief Lock the mutex. Used to prevent multiple threads from writing to the GPS at the same time
	 */
	void unlock();

	/**
	 * @brief Get the singleton instance of this class
	 */
	static AssetTrackerBase *getInstance() { return instance; }; 


protected:
	uint16_t wireReadBytesAvailable();
	int wireReadBytes(uint8_t *buf, size_t len);

	void threadFunction();
	static void threadFunctionStatic(void *param);

	TinyGPSPlus gps;
	bool useWire = false;
	TwoWire &wire = Wire;
	uint8_t wireAddr = 0x42;
	USARTSerial &serialPort = Serial1;
	Thread *thread = NULL;
	std::function<bool(char)> externalDecoder = 0;
	std::vector<std::function<void()>> threadCallbacks;
	std::vector<std::function<void()>> sentenceCallbacks;
	pin_t extIntPin = PIN_INVALID;
	os_mutex_t mutex = 0;
	static AssetTrackerBase *instance;
};

/**
 * @brief Class used for devices that use an antenna switch like the Electron AssetTracker v2
 */
class AssetTrackerAntennaSwitch {
public:
	AssetTrackerAntennaSwitch();
	virtual ~AssetTrackerAntennaSwitch();


protected:
};



/**
 * @brief Compatible replacement for the official Particle Electron AssetTracker library.
 *
 * This is an almost drop-in replacement based on my LIS3DH driver and TinyGPS++.
 * 
 * There are other classes that you can use to mix-and match features if you want to
 * used a custom design with different peripherals (see AssetTrackerFeather6).
 *
 * Note that many of the backward compatibility methods are in the LegacyAdapter class
 * so make sure you check there as well.
 */
class AssetTracker : public AssetTrackerBase, public AssetTrackerLIS3DH {

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
	 * @brief Gets the LIS3DH accelerometer object so you can access its methods directly
	 */
	LIS3DHSPI *getLIS3DHSPI();

private:
	LIS3DHSPI accel;
};

/**
 * @brief
 */
class AssetTrackerLED {
public:
	enum class BlinkState {
		OFF,
		SLOW,
		FAST,
		ON
	};

	AssetTrackerLED();
	virtual ~AssetTrackerLED();

	void setup(pin_t pin, AssetTrackerBase *base);

	void sleep();
	void wake();

	static const unsigned long SLOW_PERIOD = 500;
	static const unsigned long FAST_PERIOD = 100;

protected:
	pin_t pin; 
	AssetTrackerBase *base;
	BlinkState blinkState = BlinkState::OFF;
	bool ledState = false;
	unsigned long lastTime = 0;
};

#if PLATFORM_ID == PLATFORM_BORON
/**
 * @brief Class for the FeatherGPS6 board
 * 
 * - LIS3DH (I2C) (interrupt to MCU: D8)
 * - u-blox GNSS (I2C) (interrupt from MCU: D6)
 */
class AssetTrackerFeather6 : public AssetTrackerLED, public AssetTrackerLIS3DH, public AssetTrackerBase, public Ublox {
public:
	AssetTrackerFeather6();
	virtual ~AssetTrackerFeather6();

	void setup();

	void loop();

	/**
	 * @brief Put the GNSS into sleep mode ("BACKUP" mode)
	 * 
	 * This stops scanning but keeps the almanac and ephemeris data, so wake from sleep is fast
	 * (typically under 5 seconds).
	 * 
	 * This assumes the MCU D6 line is connected to the u-blox GNSS EXTINT pin as it's not
	 * possible to wake from backup mode by I2C only.
	 */
	bool gnssSleep();

	/**
	 * @brief Wake the GNSS from sleep mode
	 * 
	 * If you go through setup() the GNSS will automatically be wakened if it was previously
	 * asleep and you reset the device while it was asleep.
	 */
	bool gnssWake();

protected:
	LIS3DHI2C accel;

};
#endif // PLATFORM_ID == PLATFORM_BORON



#endif /* __ASSETTRACKERRK_H */
