
#include "Particle.h"
#include "AssetTrackerRK/LIS3DH.h"

// Official project location:
// https://github.com/rickkas7/LIS3DH

LIS3DHConfig::LIS3DHConfig() {
}

LIS3DHConfig &LIS3DHConfig::setLowPowerWakeMode(uint8_t movementThreshold) {
	// Enable 10 Hz, low power, with XYZ detection enabled
	reg1 = LIS3DH::CTRL_REG1_ODR1 | LIS3DH::CTRL_REG1_LPEN | LIS3DH::CTRL_REG1_ZEN | LIS3DH::CTRL_REG1_YEN | LIS3DH::CTRL_REG1_XEN;

	// Enable high-pass filter
	reg2 = LIS3DH::CTRL_REG2_FDS | LIS3DH::CTRL_REG2_HPIS1;

	// Enable INT1
	reg3 = LIS3DH::CTRL_REG3_I1_INT1;

	// Disable FIFO, enable latch interrupt on INT1_SRC
	reg5 = LIS3DH::CTRL_REG5_LIR_INT1;

	// 250 mg threshold = 16
	int1_ths = movementThreshold;

	int1_cfg = LIS3DH::INT1_CFG_YHIE_YUPE | LIS3DH::INT1_CFG_XHIE_XUPE;

	return *this;
}

LIS3DHConfig &LIS3DHConfig::setAccelMode(uint8_t rate) {

	// Enable specified rate, with XYZ detection enabled
	reg1 = rate | LIS3DH::CTRL_REG1_ZEN | LIS3DH::CTRL_REG1_YEN | LIS3DH::CTRL_REG1_XEN;

	return *this;
}

LIS3DHConfig &LIS3DHConfig::setPositionInterrupt(uint8_t movementThreshold) {
	// Enable specified rate, with XYZ detection enabled
	reg1 = LIS3DH::RATE_100_HZ | LIS3DH::CTRL_REG1_ZEN | LIS3DH::CTRL_REG1_YEN | LIS3DH::CTRL_REG1_XEN;

	// Enable INT1
	reg3 = LIS3DH::CTRL_REG3_I1_INT1;

	int1_ths = movementThreshold;

	// For position detection, enable both AOI and 6D
	int1_cfg = LIS3DH::INT1_CFG_AOI | LIS3DH::INT1_CFG_6D |
			LIS3DH::INT1_CFG_ZHIE_ZUPE | LIS3DH::INT1_CFG_ZLIE_ZDOWNE |
			LIS3DH::INT1_CFG_YHIE_YUPE | LIS3DH::INT1_CFG_YLIE_YDOWNE |
			LIS3DH::INT1_CFG_XHIE_XUPE | LIS3DH::INT1_CFG_XLIE_XDOWNE;

	return *this;
}


LIS3DH::LIS3DH(int intPin) : intPin(intPin) {

}


LIS3DH::~LIS3DH() {

}


bool LIS3DH::hasDevice() {
	bool found = false;
	for(int tries = 0; tries < 10; tries++) {
		uint8_t whoami = readRegister8(REG_WHO_AM_I);
		if (whoami == WHO_AM_I) {
			found = true;
			break;
		}
		delay(1);
	}
	return found;
}

bool LIS3DH::setup(LIS3DHConfig &config) {

	if (!hasDevice()) {
		Serial.println("device not found");
		return false;
	}

	writeRegister8(REG_CTRL_REG1, config.reg1);
	writeRegister8(REG_CTRL_REG2, config.reg2);
	writeRegister8(REG_CTRL_REG3, config.reg3);
	writeRegister8(REG_CTRL_REG4, config.reg4);
	writeRegister8(REG_CTRL_REG5, config.reg5);
	writeRegister8(REG_CTRL_REG6, config.reg6);

	if (config.setReference) {
		// In normal mode, reading the reference register sets it for the current normal force
		// (the normal force of gravity acting on the device)
		readRegister8(REG_REFERENCE);
	}
	// Set FIFO mode
	writeRegister8(REG_FIFO_CTRL_REG, config.fifoCtrlReg);



	if ((config.reg3 & CTRL_REG3_I1_INT1) != 0) {

		writeRegister8(REG_INT1_THS, config.int1_ths);
		writeRegister8(REG_INT1_DURATION, config.int1_duration);

		if (intPin >= 0) {
			// There are instructions to set the INT1_CFG in a loop in the appnote on page 24. As far
			// as I can tell this never works. Merely setting the INT1_CFG does not ever generate an
			// interrupt for me.

			// Remember the INT1_CFG setting because we're apparently supposed to set it again after
			// clearing an interrupt.
			int1_cfg = config.int1_cfg;
			writeRegister8(REG_INT1_CFG, int1_cfg);

			// Clear the interrupt just in case
			readRegister8(REG_INT1_SRC);
		}
		else {
			int1_cfg = 0;
			writeRegister8(REG_INT1_CFG, 0);
		}
	}




	return true;
}


bool LIS3DH::calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime) {
	bool ready = false;

	unsigned long start = millis();
	unsigned long lastMovement = start;
	unsigned long lastRecalibrate = start - RECALIBRATION_MOVEMENT_DELAY;

	while(maxWaitTime == 0 || millis() - start < maxWaitTime) {
		uint8_t int1_src = readRegister8(REG_INT1_SRC);
		if ((int1_src & INT1_SRC_IA) != 0) {
			Serial.printlnf("resetting lastMovement int1_src=0x%x", int1_src);
			lastMovement = lastRecalibrate = millis();
			clearInterrupt();
		}

		if (lastRecalibrate != 0 && millis() - lastRecalibrate >= RECALIBRATION_MOVEMENT_DELAY) {
			Serial.println("recalibrating");
			lastRecalibrate = 0;
			readRegister8(REG_REFERENCE);
			clearInterrupt();
		}

		if (millis() - lastMovement >= stationaryTime) {
			ready = true;
			break;
		}
	}

	return ready;
}

uint8_t LIS3DH::clearInterrupt() {
	uint8_t int1_src = readRegister8(REG_INT1_SRC);
	writeRegister8(REG_INT1_CFG, int1_cfg);

	if (intPin >= 0) {
		while(digitalRead(intPin) == HIGH) {
			delay(10);
			readRegister8(REG_INT1_SRC);
			writeRegister8(REG_INT1_CFG, int1_cfg);
		}
	}

	return int1_src;
}

void LIS3DH::enableTemperature(boolean enable) {
	writeRegister8(REG_TEMP_CFG_REG, enable ? (TEMP_CFG_TEMP_EN | TEMP_CFG_ADC_PD) : 0);
}


int16_t LIS3DH::getTemperature() {

	// https://my.st.com/public/STe2ecommunities/mcu/Lists/STM32F%20MEMS%20%20iNEMO/flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2FSTM32F%20MEMS%20%20iNEMO%2FLIS3DH%20temperature%20sensor&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580003E26E7DD54228C428E8F9FB5EE9C5185&currentviews=2886

	int16_t result = ((int16_t) readRegister16(REG_OUT_ADC3_L)) / 256;

	return result;
}


bool LIS3DH::getSample(LIS3DHSample &sample) {
	uint8_t statusAuxReg = readRegister8(REG_STATUS_AUX);

	bool hasData = ((statusAuxReg & STATUS_AUX_321DA) != 0);

	//Serial.printlnf("fifoSrcReg=0x%02x", fifoSrcReg);

	if (hasData) {
		uint8_t resp[6];
		readData(REG_OUT_X_L, resp, sizeof(resp));

		sample.x = (int16_t) (resp[0] | (((uint16_t)resp[1]) << 8));
		sample.y = (int16_t) (resp[2] | (((uint16_t)resp[3]) << 8));
		sample.z = (int16_t) (resp[4] | (((uint16_t)resp[5]) << 8));
	}
	return hasData;
}

uint8_t LIS3DH::readPositionInterrupt() {
	uint8_t pos = 0;
	uint8_t int1_src = readRegister8(REG_INT1_SRC);

	if (int1_src & INT1_SRC_IA) {
		// Clear the IA bit so we only have to test the XYZ flags
		int1_src &= ~INT1_SRC_IA;

		// See page 28 of the Application Note AN3308 for more information.
		if (int1_src == INT1_SRC_YL) {
			pos = 1; // case a
		}
		else
		if (int1_src == INT1_SRC_XH) {
			pos = 2; // case b
		}
		else
		if (int1_src == INT1_SRC_XL) {
			pos = 3; // case c
		}
		else
		if (int1_src == INT1_SRC_YH) {
			pos = 4; // case d
		}
		else
		if (int1_src == INT1_SRC_ZH) {
			pos = 5; // case e - normal case sitting flat
		}
		else
		if (int1_src == INT1_SRC_ZL) {
			pos = 6; // case f - upside down
		}
	}

	return pos;
}


uint8_t LIS3DH::readRegister8(uint8_t addr) {

	uint8_t resp[1];
	readData(addr, resp, sizeof(resp));

	return resp[0];
}

uint16_t LIS3DH::readRegister16(uint8_t addr) {

	uint8_t resp[2];
	readData(addr, resp, sizeof(resp));

	return resp[0] | (((uint16_t)resp[1]) << 8);
}


void LIS3DH::writeRegister8(uint8_t addr, uint8_t value) {
	// Serial.printlnf("writeRegister addr=%02x value=%02x", addr, value);

	uint8_t req[1];
	req[0] = value;

	writeData(addr, req, sizeof(req));
}

void LIS3DH::writeRegister16(uint8_t addr, uint16_t value) {
	// Serial.printlnf("writeRegister addr=%02x value=%04x", addr, value);

	uint8_t req[2];
	req[0] = value & 0xff;
	req[1] = value >> 8;

	writeData(addr, req, sizeof(req));
}


//
//
//

LIS3DHSPI::LIS3DHSPI(SPIClass &spi, int ss, int intPin) : LIS3DH(intPin), spi(spi), ss(ss) {

	spi.begin(ss);

	if (!spiShared) {
		spiSetup();
	}
}

LIS3DHSPI::~LIS3DHSPI() {
}

void LIS3DHSPI::spiSetup() {
	// The maximum SPI clock speed is 10 MHz. You can make it lower if needed

	spi.setBitOrder(MSBFIRST);
	spi.setClockSpeed(10, MHZ);
	spi.setDataMode(SPI_MODE0); // CPHA = 0, CPOL = 0 : MODE = 0
}

void LIS3DHSPI::beginTransaction() {

	// This doesn't work. It should, but it doesn't, and I'm not sure why.
	if (spiShared) {
		spiSetup();
		// delay(10);
	}

	digitalWrite(ss, LOW);

	// The SPI CS setup time tsu(CS) is 6 ns, should not require a delay here
}

void LIS3DHSPI::endTransaction() {
	digitalWrite(ss, HIGH);
}

bool LIS3DHSPI::readData(uint8_t addr, uint8_t *buf, size_t numBytes) {
	beginTransaction();

	if (numBytes > 1) {
		addr |= SPI_INCREMENT;
	}

	spi.transfer(SPI_READ | addr);

	for(size_t ii = 0; ii < numBytes; ii++) {
		buf[ii] = spi.transfer(0);
	}

	endTransaction();

	return true;
}

bool LIS3DHSPI::writeData(uint8_t addr, const uint8_t *buf, size_t numBytes) {
	beginTransaction();

	if (numBytes > 1) {
		addr |= SPI_INCREMENT;
	}

	spi.transfer(addr);
	for(size_t ii = 0; ii < numBytes; ii++) {
		spi.transfer(buf[ii]);
	}

	endTransaction();

	return true;
}

//
//
//

LIS3DHI2C::LIS3DHI2C(TwoWire &wire, uint8_t sad0, int intPin) : LIS3DH(intPin), wire(wire), sad0(sad0) {

}

LIS3DHI2C::~LIS3DHI2C() {

}

LIS3DHI2C::LIS3DHI2C(uint8_t sad0, int intPin) : LIS3DH(intPin), wire(Wire), sad0(sad0) {

}

bool LIS3DHI2C::readData(uint8_t addr, uint8_t *buf, size_t numBytes) {
	wire.beginTransmission(getI2CAddr());

	if (numBytes > 1) {
		addr |= I2C_INCREMENT;
	}
	wire.write(addr);

	uint8_t res = wire.endTransmission();
	if (res != 0) {
		return false;
	}

	wire.requestFrom((int)getI2CAddr(), numBytes);
	for(size_t ii = 0; ii < numBytes && wire.available(); ii++) {
		buf[ii] = wire.read();
	}
	return true;
}

bool LIS3DHI2C::writeData(uint8_t addr, const uint8_t *buf, size_t numBytes) {

	wire.beginTransmission(getI2CAddr());

	if (numBytes > 1) {
		addr |= I2C_INCREMENT;
	}
	wire.write(addr);
	for(size_t ii = 0; ii < numBytes; ii++) {
		wire.write(buf[ii]);
	}

	uint8_t res = wire.endTransmission();

	return (res == 0);
}

uint8_t LIS3DHI2C::getI2CAddr() const {
	uint8_t addr = (0b0011000 | sad0);

	return addr;
}


