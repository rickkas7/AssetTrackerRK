
#include "Particle.h"

#include "AssetTrackerRK.h"


/**
 * Compatible replacement for the official Particle AssetTracker/Electron library.
 *
 * This is an almost drop-in replacement based on my LIS3DH driver and TinyGPS++.
 *
 * If I were creating a new project, I might write directly to the LIS3DH and TinyGPS++
 * interfaces instead of using this wrapper, but it's up to you. Though the new APIs
 * available from the LegacyAdapter class are quite handy.
 *
 * Official Project Location:
 * https://github.com/rickkas7/AssetTrackerRK
 */

static const int GPS_POWER_PIN = D6;
static const int GPS_BAUD = 9600;

char emptyResponse[1] = {0};

static uint8_t internalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x00,0x00,0xF0,0x7D,0x8A,0x2A};
static uint8_t externalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x01,0x00,0xF0,0x7D,0x8B,0x2E};

AssetTrackerBase *AssetTrackerBase::instance = 0;

//
//
//
AssetTrackerLIS3DH::AssetTrackerLIS3DH(LIS3DH *accel) : accel(accel) {

}

AssetTrackerLIS3DH::~AssetTrackerLIS3DH() {

}

void AssetTrackerLIS3DH::begin(void) {
	LIS3DHConfig config;
	config.setAccelMode(LIS3DH::RATE_100_HZ);

	accel->setup(config);
}



int AssetTrackerLIS3DH::readX(void) {
	LIS3DHSample sample;
	accel->getSample(sample);
	return sample.x;
}
int AssetTrackerLIS3DH::readY(void) {
	LIS3DHSample sample;
	accel->getSample(sample);
	return sample.y;
}
int AssetTrackerLIS3DH::readZ(void) {
	LIS3DHSample sample;
	accel->getSample(sample);
	return sample.z;
}

int AssetTrackerLIS3DH::readXYZmagnitude(void) {
	LIS3DHSample sample;
	accel->getSample(sample);

    int magnitude = sqrt((sample.x*sample.x)+(sample.y*sample.y)+(sample.z*sample.z));
    return magnitude;
}

bool AssetTrackerLIS3DH::setupLowPowerWakeMode(uint8_t movementThreshold) {
	LIS3DHConfig config;
	config.setLowPowerWakeMode(movementThreshold);

	return accel->setup(config);
}

uint8_t AssetTrackerLIS3DH::clearAccelInterrupt() {
	return accel->clearInterrupt();
}

bool AssetTrackerLIS3DH::calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime) {
	return accel->calibrateFilter(stationaryTime, maxWaitTime);
}

//
//
//
AssetTrackerBase::AssetTrackerBase() : LegacyAdapter(gps) {
	instance = this;
}

AssetTrackerBase::~AssetTrackerBase() {
	if (mutex) {
		os_mutex_destroy(mutex);
		mutex = 0;
	}
}

void AssetTrackerBase::begin() {
	if (!mutex) {
		os_mutex_create(&mutex);
	}
	if (!useWire) {
		serialPort.begin(GPS_BAUD);
	}
	if (extIntPin != PIN_INVALID) {
		pinMode(extIntPin, OUTPUT);
		digitalWrite(extIntPin, HIGH);
	}
}


void AssetTrackerBase::updateGPS(void) {
	bool hasSentence = false;

	if (!useWire) {
		while (serialPort.available() > 0) {
			char c = (char)serialPort.read();
			hasSentence |= gps.encode(c);
			if (externalDecoder) {
				externalDecoder(c);
			}
		}
	}
	else {
		uint8_t buf[32];

		WITH_LOCK(wire) {
			uint16_t available = wireReadBytesAvailable();
			if (available > 32) {
				available = 32;
			}
			if (available > 0) {
				if (wireReadBytes(buf, available) == available) {
					for(uint16_t ii = 0; ii < available; ii++) {
						hasSentence |= gps.encode(buf[ii]);
						if (externalDecoder) {
							externalDecoder(buf[ii]);
						}
					}
					// Log.info("got %d bytes from GNSS", available);
					// Log.dump(buf, available);
				}
			}
		}
	}
	if (hasSentence) {
		for(auto it = sentenceCallbacks.begin(); it != sentenceCallbacks.end(); it++) {
			(*it)();
		}
	}
}


void AssetTrackerBase::sendCommand(const uint8_t *cmd, size_t len) {

	/*
	Log.info("sendCommand len=%u", len);
	Log.dump(cmd, len);
	Log.print("\r\n");
	 */

	WITH_LOCK(*this) {
		if (!useWire) {
			serialPort.write(cmd, len);
		}
		else {
			WITH_LOCK(wire) {
				size_t offset = 0;

				while(offset < len) {
					// uint8_t res;

					size_t reqLen = (len - offset);
					if (reqLen > 32) {
						reqLen = 32;
					}

					wire.beginTransmission(wireAddr);

					wire.write(&cmd[offset], reqLen);

					offset += reqLen;

					/* res =*/ wire.endTransmission((offset >= len));
				}
			}
		}
	}

}

void AssetTrackerBase::startThreadedMode() {
	if (thread == NULL) {
		thread = new Thread("AssetTracker", threadFunctionStatic, this, OS_THREAD_PRIORITY_DEFAULT, 2048);
	}
}

void AssetTrackerBase::threadFunction() {
	while(true) {
		updateGPS();

		for(auto it = threadCallbacks.begin(); it != threadCallbacks.end(); it++) {
			(*it)();
		}
		os_thread_yield();
	}
}

// [static]
void AssetTrackerBase::threadFunctionStatic(void *param) {
	static_cast<AssetTracker *>(param)->threadFunction();
}

void AssetTrackerBase::lock() { 
	if (mutex == 0) {
		os_mutex_create(&mutex);
	}

	os_mutex_lock(mutex); 
};

void AssetTrackerBase::unlock() { 
	os_mutex_unlock(mutex); 
};


AssetTrackerBase &AssetTrackerBase::withSerialPort(USARTSerial &port) {
	useWire = false;
	serialPort = port;
	return *this;
}

AssetTrackerBase &AssetTrackerBase::withI2C(TwoWire &wire, uint8_t addr) {
	useWire = true;
	this->wire = wire;
	this->wireAddr = addr;

	wire.begin();

	return *this;
}


uint16_t AssetTrackerBase::wireReadBytesAvailable() {
	uint8_t res;

	wire.beginTransmission(wireAddr);
	wire.write(0xfd);
	res = wire.endTransmission(false);
	if (res != 0) {
		// Log.info("wireReadBytesAvailable I2C error %u", res);
		return 0;
	}

	res = wire.requestFrom(wireAddr, (uint8_t) 2, (uint8_t) true);
	if (res != 2) {
		// Log.info("wireReadBytesAvailable incorrect count %u", res);
		return 0;
	}

	uint16_t available = wire.read() << 8;
	available |= wire.read();

	return available;
}

int AssetTrackerBase::wireReadBytes(uint8_t *buf, size_t len) {
	uint8_t res;

	// Log.info("wireReadBytes len=%u", len);

	wire.beginTransmission(wireAddr);
	wire.write(0xff);
	res = wire.endTransmission(false);
	if (res != 0) {
		// Log.info("wireReadBytes I2C error %u", res);
		return -1;
	}

	size_t offset = 0;

	while(offset < len) {
		size_t reqLen = (len - offset);
		if (reqLen > 32) {
			reqLen = 32;
		}
		res = wire.requestFrom(wireAddr, (uint8_t) reqLen, (uint8_t) ((offset + reqLen) == len));
		if (res != reqLen) {
			// Log.info("wireReadBytes incorrect count %u", res);
			return -1;
		}

		for(size_t ii = 0; ii < reqLen; ii++) {
			buf[offset + ii] = wire.read();
		}
		offset += reqLen;
 	}
	return len;
}



//
//
//
AssetTracker::AssetTracker() : AssetTrackerLIS3DH(&accel), accel(SPI, A2, WKP) {
}

AssetTracker::~AssetTracker() {

}

void AssetTracker::begin(void) {
	AssetTrackerLIS3DH::begin();
}

void AssetTracker::gpsOn(void) {
	AssetTrackerBase::begin();

    pinMode(GPS_POWER_PIN, OUTPUT);
    digitalWrite(GPS_POWER_PIN, LOW);
}

void AssetTracker::gpsOff(void) {
    digitalWrite(GPS_POWER_PIN, HIGH);
}


bool AssetTracker::antennaInternal() {
	sendCommand(internalANT, sizeof(internalANT));
	return true;
}

bool AssetTracker::antennaExternal() {
	sendCommand(externalANT, sizeof(externalANT));
	return true;
}


char *AssetTracker::preNMEA(void) {
	return emptyResponse;
}

LIS3DHSPI *AssetTracker::getLIS3DHSPI() {
	return &accel;
}


//
//
//
AssetTrackerLED::AssetTrackerLED() {

}
AssetTrackerLED::~AssetTrackerLED() {
}

void AssetTrackerLED::setup(pin_t pin_, AssetTrackerBase *base_) {
	pin = pin_;
	base = base_;
	
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);

	base->setSentenceCallback([this]() {
		if (base->gpsFix()) {
			blinkState = BlinkState::ON;
		}
		else {
			if (base->getSatellites() > 3) {
				blinkState = BlinkState::FAST;
			}
			else {
				blinkState = BlinkState::SLOW;
			}

		}
	});

	base->setThreadCallback([this]() {
		switch(blinkState) {
		case BlinkState::OFF:
			if (ledState) {
				digitalWrite(pin, LOW);
				ledState = false;
			}
			break;

		case BlinkState::SLOW:
			if (millis() - lastTime >= SLOW_PERIOD) {
				lastTime = millis();

				ledState = !ledState;
				digitalWrite(pin, ledState);
			}
			break;

		case BlinkState::FAST:
			if (millis() - lastTime >= FAST_PERIOD) {
				lastTime = millis();

				ledState = !ledState;
				digitalWrite(pin, ledState);
			}
			break;

		case BlinkState::ON:

			if (!ledState) {
				digitalWrite(pin, HIGH);
				ledState = true;
			}
			break;
		}
	});
}

void AssetTrackerLED::sleep() {
	blinkState = BlinkState::OFF;
	digitalWrite(pin, LOW);
}

void AssetTrackerLED::wake() {
}


#if PLATFORM_ID == PLATFORM_BORON
//
//
//
AssetTrackerFeather6::AssetTrackerFeather6() : AssetTrackerLIS3DH(&accel), accel(Wire, 0, D8)  {

}

AssetTrackerFeather6::~AssetTrackerFeather6() {

}

void AssetTrackerFeather6::setup() {
	// AssetTrackerBase configuration
	AssetTrackerBase::withI2C(Wire, 0x42);
	AssetTrackerBase::withGNSSExtInt(D6);

	AssetTrackerLIS3DH::begin();
	AssetTrackerBase::begin();

	AssetTrackerBase::startThreadedMode();

	Ublox::setup();

	// Since we use threaded mode, also call Ublox::loop() from the thread so blocking
	// instructions from loop won't get stuck forever.
	setThreadCallback([this]() {
		Ublox::loop();
	});

	// D6 is connected to EXTINT on the u-blox GNSS
	// It's set up so if LOW, the GNSS goes into BACKUP mode where it stops scanning and
	// enters low-power mode but keeps the almanac and ephemeris data. This allows for
	// fast wake from sleep (typically under 5 seconds).
	pinMode(D6, OUTPUT);
	digitalWrite(D6, HIGH);

	// A5 is used as the GNSS lock LED
	AssetTrackerLED::setup(A5, this);	
}

void AssetTrackerFeather6::loop() {
}

bool AssetTrackerFeather6::gnssSleep() {

	Ublox::enableExtIntBackupSync(true);
	AssetTrackerLED::sleep();

	digitalWrite(D6, LOW);

	return true;
}

bool AssetTrackerFeather6::gnssWake() {

	digitalWrite(D6, HIGH);
	AssetTrackerLED::wake();

	return true;
}

#endif // PLATFORM_ID == PLATFORM_BORON
