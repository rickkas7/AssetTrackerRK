
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

static LIS3DHSPI accel(SPI, A2, WKP);
static TinyGPSPlus gps;
char emptyResponse[1] = {0};

static uint8_t internalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x00,0x00,0xF0,0x7D,0x8A,0x2A};
static uint8_t externalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x01,0x00,0xF0,0x7D,0x8B,0x2E};

AssetTracker *AssetTracker::instance = 0;


AssetTracker::AssetTracker() : LegacyAdapter(gps) {
	instance = this;
}

AssetTracker::~AssetTracker() {

}

void AssetTracker::begin(void) {
	LIS3DHConfig config;
	config.setAccelMode(LIS3DH::RATE_100_HZ);

	accel.setup(config);
}

void AssetTracker::updateGPS(void) {
	if (!useWire) {
		while (serialPort.available() > 0) {
			char c = (char)serialPort.read();
			gps.encode(c);
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
						gps.encode(buf[ii]);
						if (externalDecoder) {
							externalDecoder(buf[ii]);
						}
					}
				}
			}
		}
	}
}

void AssetTracker::startThreadedMode() {
	if (thread == NULL) {
		thread = new Thread("AssetTracker", threadFunctionStatic, this, OS_THREAD_PRIORITY_DEFAULT, 2048);
	}
}

void AssetTracker::threadFunction() {
	while(true) {
		updateGPS();
		os_thread_yield();
	}
}

// [static]
void AssetTracker::threadFunctionStatic(void *param) {
	static_cast<AssetTracker *>(param)->threadFunction();
}

void AssetTracker::gpsOn(void) {
	if (!mutex) {
		os_mutex_create(&mutex);
	}
	if (!useWire) {
		serialPort.begin(GPS_BAUD);
	}
    pinMode(GPS_POWER_PIN, OUTPUT);
    digitalWrite(GPS_POWER_PIN, LOW);
}

void AssetTracker::gpsOff(void) {
    digitalWrite(GPS_POWER_PIN, HIGH);
}

void AssetTracker::sendCommand(const uint8_t *cmd, size_t len) {

	/*
	Log.info("sendCommand len=%u", len);
	Log.dump(cmd, len);
	Log.print("\r\n");
	 */

	if (!mutex) {
		// This is normally created in gpsOn but this is here for safety in case it's not called
		os_mutex_create(&mutex);
	}

	WITH_LOCK(*this) {
		if (!useWire) {
			serialPort.write(cmd, len);
		}
		else {
			WITH_LOCK(wire) {
				size_t offset = 0;

				while(offset < len) {
					uint8_t res;

					size_t reqLen = (len - offset);
					if (reqLen > 32) {
						reqLen = 32;
					}

					wire.beginTransmission(wireAddr);

					wire.write(&cmd[offset], reqLen);

					offset += reqLen;

					res = wire.endTransmission((offset >= len));
				}
			}
		}
	}

}

bool AssetTracker::antennaInternal() {
	sendCommand(internalANT, sizeof(internalANT));
	return true;
}

bool AssetTracker::antennaExternal() {
	sendCommand(externalANT, sizeof(externalANT));
	return true;
}

int AssetTracker::readX(void) {
	LIS3DHSample sample;
	accel.getSample(sample);
	return sample.x;
}
int AssetTracker::readY(void) {
	LIS3DHSample sample;
	accel.getSample(sample);
	return sample.y;
}
int AssetTracker::readZ(void) {
	LIS3DHSample sample;
	accel.getSample(sample);
	return sample.z;
}

int AssetTracker::readXYZmagnitude(void) {
	LIS3DHSample sample;
	accel.getSample(sample);

    int magnitude = sqrt((sample.x*sample.x)+(sample.y*sample.y)+(sample.z*sample.z));
    return magnitude;
}

char *AssetTracker::preNMEA(void) {
	return emptyResponse;
}

bool AssetTracker::setupLowPowerWakeMode(uint8_t movementThreshold) {
	LIS3DHConfig config;
	config.setLowPowerWakeMode(movementThreshold);

	return accel.setup(config);
}

uint8_t AssetTracker::clearAccelInterrupt() {
	return accel.clearInterrupt();
}

bool AssetTracker::calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime) {
	return accel.calibrateFilter(stationaryTime, maxWaitTime);
}

AssetTracker &AssetTracker::withSerialPort(USARTSerial &port) {
	useWire = false;
	serialPort = port;
	return *this;
}

AssetTracker &AssetTracker::withI2C(TwoWire &wire, uint8_t addr) {
	useWire = true;
	this->wire = wire;
	this->wireAddr = addr;

	wire.begin();

	return *this;
}

uint16_t AssetTracker::wireReadBytesAvailable() {
	uint8_t res;

	wire.beginTransmission(wireAddr);
	wire.write(0xfd);
	res = wire.endTransmission(false);
	if (res != 0) {
		Log.info("wireReadBytesAvailable I2C error %u", res);
		return 0;
	}

	res = wire.requestFrom(wireAddr, (uint8_t) 2, (uint8_t) true);
	if (res != 2) {
		Log.info("wireReadBytesAvailable incorrect count %u", res);
		return 0;
	}

	uint16_t available = wire.read() << 8;
	available |= wire.read();

	return available;
}

int AssetTracker::wireReadBytes(uint8_t *buf, size_t len) {
	uint8_t res;

	// Log.info("wireReadBytes len=%u", len);

	wire.beginTransmission(wireAddr);
	wire.write(0xff);
	res = wire.endTransmission(false);
	if (res != 0) {
		Log.info("wireReadBytes I2C error %u", res);
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
			Log.info("wireReadBytes incorrect count %u", res);
			return -1;
		}

		for(size_t ii = 0; ii < reqLen; ii++) {
			buf[offset + ii] = wire.read();
		}
		offset += reqLen;
 	}
	return len;
}


LIS3DHSPI *AssetTracker::getLIS3DHSPI() {
	return &accel;
}

TinyGPSPlus *AssetTracker::getTinyGPSPlus() {
	return &gps;
}


