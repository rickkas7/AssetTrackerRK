#ifndef __LIS3DH_H
#define __LIS3DH_H

// Interface to the LIS3DH accelerometer used on the Particle AssetTracker for Electron
// Also supports standalone devices connected by SPI or I2C
//
// Data sheet:
// http://www.st.com/resource/en/datasheet/lis3dh.pdf
//
// Extremely helpful application note:
// http://www.st.com/resource/en/application_note/cd00290365.pdf
//
// Official project location:
// https://github.com/rickkas7/LIS3DH

/**
 * Structure to hold a single XYZ sample
 */
typedef struct {
	int16_t 	x;
	int16_t		y;
	int16_t		z;
} LIS3DHSample;


/**
 * Configuration object. You normally create one of these on the stack and then use
 * one of the set* methods to set the mode you want to use. The object then gets
 * passed to the setup() method. The configuration object does not need to exist
 * after the setup method returns.
 *
 * This provides a way of adjusting the low-level settings. You can also use one
 * of the setup method and tweak the settings before calling setup.
 */
class LIS3DHConfig {
public:
	LIS3DHConfig();

	LIS3DHConfig &setLowPowerWakeMode(uint8_t movementThreshold);
	LIS3DHConfig &setAccelMode(uint8_t rate);
	LIS3DHConfig &setPositionInterrupt(uint8_t movementThreshold);

	uint8_t reg1 = 0;
	uint8_t reg2 = 0;
	uint8_t reg3 = 0;
	uint8_t reg4 = 0;
	uint8_t reg5 = 0;
	uint8_t reg6 = 0;
	bool	setReference = false;
	uint8_t int1_ths = 0;
	uint8_t int1_duration = 0;
	uint8_t int1_cfg = 0;
	uint8_t fifoCtrlReg = 0;
};

/**
 * The LIS3DH accelerometer object base class. You cannot instantiate one of these
 * directly; you should instantiate either a LIS3DHSPI or LIS3DHI2C depending
 * which interface you have connected.
 */
class LIS3DH {
public:
	LIS3DH(int intPin = -1);
	virtual ~LIS3DH();

	/**
	 * Returns true if the device can be found on the I2C or SPI bus. Uses
	 * the WHOAMI register.
	 */
	bool hasDevice();

	/**
	 * Initializes the device
	 *
	 * Normally you instantiate a LIS3DHConfig object on the stack and initialize it using
	 * setupLowPowerWakeMode, setAccelMode, or setPositionInterrupt and then pass the
	 * object to this method, possibly adjust the configuration as necessary.
	 */
	bool setup(LIS3DHConfig &config);


	/**
	 * The movement interrupt needs to be calculated to take into account gravity and its
	 * current orientation. For this to work, the object must be stationary. You typically
	 * call this method before putting the device to sleep for wake-on-move. The stationnaryTime
	 * is the amount of time the device needs to be stationary, in milliseconds. Typically, you'd
	 * wait a few seconds. The maxWaitTime is the amount of time in milliseconds to wait for
	 * the device to stop moving. 0 means wait forever.
	 */
	bool calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime = 0);

	/**
	 * After getting an interrupt, call this to read the interrupt status and clear the interrupt
	 * The resulting value is the value of the INT1_SRC register.
	 */
	uint8_t clearInterrupt();

	/**
	 * Enables the temperature sensor. Call once, usually after calling setup.
	 */
	void enableTemperature(boolean enable = true);

	/**
	 * Gets the temperature of the sensor in degrees C
	 *
	 * Make sure you call enableTemperature() at when you're setting the modes, because it seems
	 * to take a while to start up, you can't just keep turning it on and off all the time,
	 * apparently.
	 *
	 * Also, the temperature sensor is horribly inaccurate.
	 */
	int16_t getTemperature();

	/**
	 * Get sample is used when querying x, y, z data not using the FIFO.
	 *
	 * The data is still only captured at the data capture rate, and this method will return false
	 * if there is no new data to be read.
	 */
	bool getSample(LIS3DHSample &sample);

	/**
	 * When using position interrupt mode, returns the orientation of the device.
	 *
	 * 0 = not in a known position
	 * 1 = position a
	 * 2 = position b
	 * 3 = position c
	 * 4 = position d
	 * 5 = position e = normal position facing up
	 * 6 = position f = upside down
	 *
	 * See page 28 of the Application Note AN3308 for more information.
	 */
	uint8_t readPositionInterrupt();

	/**
	 * Reads an 8-bit register value
	 *
	 * Most of the calls have easier to use accessors like readStatus() that use this call internally.
	 *
	 * addr: One of the register addresses, such as REG_STATUS
	 */
	uint8_t readRegister8(uint8_t addr);

	/**
	 * Reads an 16-bit register value
	 *
	 * addr: One of the register addresses, such as REG_THRESH_ACT_L. It must be the first of a pair of _L and _H registers.
	 */
	uint16_t readRegister16(uint8_t addr);

	/**
	 * Write an 8-bit register value
	 *
	 * Most of the calls have easier to use accessors like writeIntmap1() that use this call internally.
	 *
	 * addr: One of the register addresses, such as REG_INTMAP1
	 */
	void writeRegister8(uint8_t addr, uint8_t value);

	/**
	 * Write an 16-bit register value
	 *
	 * Most of the calls have easier to use accessors like writeIntmap1() that use this call internally.
	 *
	 * addr: One of the register addresses, such as REG_THRESH_ACT_L. It must be the first of a pair of _L and _H registers.
	 */
	void writeRegister16(uint8_t addr, uint16_t value);


	/**
	 * Low-level read data from the device. Implemented by subclasses.
	 *
	 * Normally you'd use the readRegister calls.
	 */
	virtual bool readData(uint8_t addr, uint8_t *buf, size_t numBytes) = 0;

	/**
	 * Low-level write data from the device. Implemented by subclasses.
	 *
	 * Normally you'd use the writeRegister calls.
	 */
	virtual bool writeData(uint8_t addr, const uint8_t *buf, size_t numBytes) = 0;


	static const uint8_t WHO_AM_I = 0b00110011;

	static const uint8_t REG_STATUS_AUX = 0x07;
	static const uint8_t REG_OUT_ADC1_L = 0x08;
	static const uint8_t REG_OUT_ADC1_H = 0x09;
	static const uint8_t REG_OUT_ADC2_L = 0x0a;
	static const uint8_t REG_OUT_ADC2_H = 0x0b;
	static const uint8_t REG_OUT_ADC3_L = 0x0c;
	static const uint8_t REG_OUT_ADC3_H = 0x0d;
	static const uint8_t REG_INT_COUNTER_REG = 0x0e;
	static const uint8_t REG_WHO_AM_I = 0x0f;
	static const uint8_t REG_TEMP_CFG_REG = 0x1f;
	static const uint8_t REG_CTRL_REG1 = 0x20;
	static const uint8_t REG_CTRL_REG2 = 0x21;
	static const uint8_t REG_CTRL_REG3 = 0x22;
	static const uint8_t REG_CTRL_REG4 = 0x23;
	static const uint8_t REG_CTRL_REG5 = 0x24;
	static const uint8_t REG_CTRL_REG6 = 0x25;
	static const uint8_t REG_REFERENCE = 0x26;
	static const uint8_t REG_STATUS_REG = 0x27;
	static const uint8_t REG_OUT_X_L = 0x28;
	static const uint8_t REG_OUT_X_H = 0x29;
	static const uint8_t REG_OUT_Y_L = 0x2a;
	static const uint8_t REG_OUT_Y_H = 0x2b;
	static const uint8_t REG_OUT_Z_L = 0x2c;
	static const uint8_t REG_OUT_Z_H = 0x2d;
	static const uint8_t REG_FIFO_CTRL_REG = 0x2e;
	static const uint8_t REG_FIFO_SRC_REG = 0x2f;
	static const uint8_t REG_INT1_CFG = 0x30;
	static const uint8_t REG_INT1_SRC = 0x31;
	static const uint8_t REG_INT1_THS = 0x32;
	static const uint8_t REG_INT1_DURATION = 0x33;
	static const uint8_t REG_CLICK_CFG = 0x38;
	static const uint8_t REG_CLICK_SRC = 0x39;
	static const uint8_t REG_CLICK_THS = 0x3a;
	static const uint8_t REG_TIME_LIMIT = 0x3b;
	static const uint8_t REG_TIME_LATENCY = 0x3c;
	static const uint8_t REG_TIME_WINDOW = 0x3d;

	static const uint8_t STATUS_AUX_321OR = 0x80;
	static const uint8_t STATUS_AUX_3OR = 0x40;
	static const uint8_t STATUS_AUX_2OR = 0x20;
	static const uint8_t STATUS_AUX_1OR = 0x10;
	static const uint8_t STATUS_AUX_321DA = 0x08;
	static const uint8_t STATUS_AUX_3DA = 0x04;
	static const uint8_t STATUS_AUX_2DA = 0x02;
	static const uint8_t STATUS_AUX_1DA = 0x01;

	static const uint8_t CTRL_REG1_ODR3 = 0x80;
	static const uint8_t CTRL_REG1_ODR2 = 0x40;
	static const uint8_t CTRL_REG1_ODR1 = 0x20;
	static const uint8_t CTRL_REG1_ODR0 = 0x10;
	static const uint8_t CTRL_REG1_LPEN = 0x08;
	static const uint8_t CTRL_REG1_ZEN = 0x04;
	static const uint8_t CTRL_REG1_YEN = 0x02;
	static const uint8_t CTRL_REG1_XEN = 0x01;

	static const uint8_t RATE_1_HZ   = 0x10;
	static const uint8_t RATE_10_HZ  = 0x20;
	static const uint8_t RATE_25_HZ  = 0x30;
	static const uint8_t RATE_50_HZ  = 0x40;
	static const uint8_t RATE_100_HZ = 0x50;
	static const uint8_t RATE_200_HZ = 0x60;
	static const uint8_t RATE_400_HZ = 0x70;

	static const uint8_t CTRL_REG2_HPM1 = 0x80;
	static const uint8_t CTRL_REG2_HPM0 = 0x40;
	static const uint8_t CTRL_REG2_HPCF2 = 0x20;
	static const uint8_t CTRL_REG2_HPCF1 = 0x10;
	static const uint8_t CTRL_REG2_FDS = 0x08;
	static const uint8_t CTRL_REG2_HPCLICK = 0x04;
	static const uint8_t CTRL_REG2_HPIS2 = 0x02;
	static const uint8_t CTRL_REG2_HPIS1 = 0x01;


	static const uint8_t CTRL_REG3_I1_CLICK = 0x80;
	static const uint8_t CTRL_REG3_I1_INT1 = 0x40;
	static const uint8_t CTRL_REG3_I1_DRDY = 0x10;
	static const uint8_t CTRL_REG3_I1_WTM = 0x04;
	static const uint8_t CTRL_REG3_I1_OVERRUN = 0x02;

	static const uint8_t CTRL_REG4_BDU = 0x80;
	static const uint8_t CTRL_REG4_BLE = 0x40;
	static const uint8_t CTRL_REG4_FS1 = 0x20;
	static const uint8_t CTRL_REG4_FS0 = 0x10;
	static const uint8_t CTRL_REG4_HR = 0x08;
	static const uint8_t CTRL_REG4_ST1 = 0x04;
	static const uint8_t CTRL_REG4_ST0 = 0x02;
	static const uint8_t CTRL_REG4_SIM = 0x01;


	static const uint8_t CTRL_REG5_BOOT = 0x80;
	static const uint8_t CTRL_REG5_FIFO_EN = 0x40;
	static const uint8_t CTRL_REG5_LIR_INT1 = 0x08;
	static const uint8_t CTRL_REG5_D4D_INT1 = 0x04;

	static const uint8_t CTRL_REG6_I2_CLICK = 0x80;
	static const uint8_t CTRL_REG6_I2_INT2 = 0x40;
	static const uint8_t CTRL_REG6_BOOT_I2 = 0x10;
	static const uint8_t CTRL_REG6_H_LACTIVE = 0x02;

	static const uint8_t INT1_CFG_AOI = 0x80;
	static const uint8_t INT1_CFG_6D = 0x40;
	static const uint8_t INT1_CFG_ZHIE_ZUPE = 0x20;
	static const uint8_t INT1_CFG_ZLIE_ZDOWNE = 0x10;
	static const uint8_t INT1_CFG_YHIE_YUPE = 0x08;
	static const uint8_t INT1_CFG_YLIE_YDOWNE = 0x04;
	static const uint8_t INT1_CFG_XHIE_XUPE = 0x02;
	static const uint8_t INT1_CFG_XLIE_XDOWNE = 0x01;

	static const uint8_t INT1_SRC_IA = 0x40;
	static const uint8_t INT1_SRC_ZH = 0x20;
	static const uint8_t INT1_SRC_ZL = 0x10;
	static const uint8_t INT1_SRC_YH = 0x08;
	static const uint8_t INT1_SRC_YL = 0x04;
	static const uint8_t INT1_SRC_XH = 0x02;
	static const uint8_t INT1_SRC_XL = 0x01;

	static const uint8_t TEMP_CFG_ADC_PD = 0x80;
	static const uint8_t TEMP_CFG_TEMP_EN = 0x40;

	static const uint8_t FIFO_CTRL_BYPASS = 0x00;
	static const uint8_t FIFO_CTRL_FIFO = 0x40;
	static const uint8_t FIFO_CTRL_STREAM = 0x80;
	static const uint8_t FIFO_CTRL_STREAM_TO_FIFO = 0xc0;

	static const uint8_t FIFO_SRC_WTM = 0x80;
	static const uint8_t FIFO_SRC_OVRN = 0x40;
	static const uint8_t FIFO_SRC_EMPTY = 0x20;
	static const uint8_t FIFO_SRC_FSS_MASK = 0x1f;

	static const unsigned long RECALIBRATION_MOVEMENT_DELAY = 100;

private:
	int intPin; // Pin connected to INT1 on the accelerometer (-1 = not connected)
	uint8_t int1_cfg; // What we set as INT1_CFG
};

/**
 * Implementation of the SPI interface to the LIS3DH
 */
class LIS3DHSPI : public LIS3DH {
public:
	/**
	 * Initialize the LIS3DH on an SPI port
	 *
	 * spi: specifies the SPI port (SPI, SPI1, etc.)
	 * ss: specifies the CS pin
	 *
	 * If the INT pin is connected to a pin, specify which one with intPin
	 */
	LIS3DHSPI(SPIClass &spi, int ss = A2, int intPin = -1);
	virtual ~LIS3DHSPI();

	virtual bool readData(uint8_t addr, uint8_t *buf, size_t numBytes);

	virtual bool writeData(uint8_t addr, const uint8_t *buf, size_t numBytes);

	static const uint8_t SPI_READ = 0x80;
	static const uint8_t SPI_INCREMENT= 0x40;


private:
	void spiSetup();

	virtual void beginTransaction();
	virtual void endTransaction();

	SPIClass &spi; // Typically SPI or SPI1
	int ss;		// SS or /CS chip select pin. Default: A2, -1 if using I2C
	bool spiShared = false;

};

/**
 * Implementation of the I2C interface to the LIS3DH
 */
class LIS3DHI2C : public LIS3DH {
public:
	/**
	 * Initialize the LIS3DH on the any I2C port
	 *
	 * For the default port (Wire):
	 * SCL: Connect to D1 (I2C SCL)
	 * SDA: Connect to D0 (I2C SDA)
	 *
	 * For the Wire1 (Electron only):
	 * SCL: Connect to C5 (I2C SCL)
	 * SDA: Connect to D4 (I2C SDA)
	 *
	 * If the SDO pin is not connected or connected to ground, set sad0 to 0 (address 0x18)
	 * If the SDO pin is connected to 3V3, set sad0 to 1 (address 0x19)
	 *
	 * If the INT pin is connected to a pin, specify which one with intPin
	 */
	LIS3DHI2C(TwoWire &wire, uint8_t sad0 = 0, int intPin = -1);
	virtual ~LIS3DHI2C();

	/**
	 * Initialize the LIS3DH on the default I2C port (D0/D1)
	 *
	 * Connect:
	 * SCL: Connect to D1 (I2C SCL)
	 * SDA: Connect to D0 (I2C SDA)
	 *
	 * If the SDO pin is not connected or connected to ground, set sad0 to 0 (address 0x18)
	 * If the SDO pin is connected to 3V3, set sad0 to 1 (address 0x19)
	 *
	 * If the INT pin is connected to a pin, specify which one with intPin
	 */
	LIS3DHI2C(uint8_t sad0 = 0, int intPin = -1);

	virtual bool readData(uint8_t addr, uint8_t *buf, size_t numBytes);

	virtual bool writeData(uint8_t addr, const uint8_t *buf, size_t numBytes);

	static const uint8_t I2C_INCREMENT= 0x80;

private:
	uint8_t getI2CAddr() const;

	TwoWire &wire; // Wire or Wire1, when using I2C mode
	uint8_t sad0; // 0 or 1 depending on the state of the SD0/SA0 pin
};



#endif /* __LIS30DH_H */
