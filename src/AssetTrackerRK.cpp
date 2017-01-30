
#include "Particle.h"

#include "AssetTrackerRK.h"

/**
 * Compatible replacement for the official Particle AssetTracker/Electron library.
 *
 * This is an almost drop-in replacement based on my LIS3DH driver and TinyGPS++.
 *
 * If I were creating a new project, I'd write directly to the LIS3DH and TinyGPS++
 * interfaces instead of using this wrapper, but it's up to you.
 *
 * Official Project Location:
 * https://github.com/rickkas7/AssetTrackerRK
 */

static const int GPS_POWER_PIN = D6;
static const int GPS_BAUD = 9600;

static LIS3DHSPI accel(SPI, A2, WKP);
static TinyGPSPlus gps;
char emptyResponse[1] = {0};

AssetTracker::AssetTracker() {

}

void AssetTracker::begin(void) {
	LIS3DHConfig config;
	config.setAccelMode(LIS3DH::RATE_100_HZ);

	accel.setup(config);
}

void AssetTracker::updateGPS(void) {
	while (Serial1.available() > 0) {
		if (gps.encode(Serial1.read())) {
		}
	}
}

void AssetTracker::gpsOn(void) {
	Serial1.begin(GPS_BAUD);
    pinMode(GPS_POWER_PIN, OUTPUT);
    digitalWrite(GPS_POWER_PIN, LOW);
}

void AssetTracker::gpsOff(void) {
    digitalWrite(GPS_POWER_PIN, HIGH);
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

float AssetTracker::readLat(void) {
	return (float) gps.location.lat();
}
float AssetTracker::readLon(void) {
	return (float) gps.location.lng();
}

bool AssetTracker::gpsFix(void) {
	return gps.location.isValid();
}

char *AssetTracker::preNMEA(void) {
	return emptyResponse;
}


String AssetTracker::readLatLon(void) {
    String latLon = String::format("%f,%f",gps.location.lat(), gps.location.lng());
    return latLon;

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

LIS3DHSPI *AssetTracker::getLIS3DHSPI() {
	return &accel;
}

TinyGPSPlus *AssetTracker::getTinyGPSPlus() {
	return &gps;
}


