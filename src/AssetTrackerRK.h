#ifndef __ASSETTRACKERRK_H
#define __ASSETTRACKERRK_H

#include "LIS3DH.h"
#include "TinyGPS++.h"
#include "LegacyAdapter.h"

/**
 * Compatible replacement for the official Particle AssetTracker/Electron library.
 *
 * This is an almost drop-in replacement based on my LIS3DH driver and TinyGPS++.
 *
 * If I were creating a new project, I'd write directly to the LIS3DH and TinyGPS++
 * interfaces instead of using this wrapper, but it's up to you.
 */
class AssetTracker : public LegacyAdapter {

public:
	AssetTracker();

	void begin(void);
	void updateGPS(void);
	void gpsOn(void);
	void gpsOff(void);


	int readX(void);
	int readY(void);
	int readZ(void);
	int readXYZmagnitude(void);

	static bool antennaInternal();
	static bool antennaExternal();

	// This isn't supported, because I wasn't sure how to get it out of TinyGPS++
	char *preNMEA(void);


	bool setupLowPowerWakeMode(uint8_t movementThreshold = 16);
	uint8_t clearAccelInterrupt();
	bool calibrateFilter(unsigned long stationaryTime, unsigned long maxWaitTime = 0);

	LIS3DHSPI *getLIS3DHSPI();
	TinyGPSPlus *getTinyGPSPlus();

	static const unsigned long MAX_GPS_AGE_MS = 10000; // GPS location must be newer than this to be considered valid

private:
};

#endif /* __ASSETTRACKERRK_H */
