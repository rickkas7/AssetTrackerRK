#ifndef __LEGACYADAPTER_H
#define __LEGACYADAPTER_H

#include "TinyGPS++.h"

/**
 * This class adapts the output from TinyGPS++ to match the results from the Adafruit parser and Particle AssetTracker v1 library
 */
class LegacyAdapter {
public:
	/**
	 * @brief Constructs the LegacyAdapter object
	 *
	 * @param gpsData The TinyGPSPlus object containing the actual data
	 */
	LegacyAdapter(TinyGPSPlus &gpsData);

	/**
	 * @brief Destructor
	 */
	virtual ~LegacyAdapter();

	/**
	 * @brief Converts a degree value (as a double) into the weird GPS DDMM.MMMMM format (degrees * 100 + minutes).
	 *
	 * This is the format for the Adafruit methods that don't have "Deg" in them, like readLat() and readLon().
	 * Because of rounding it may not be the same as the actual value returned by the GPS, but it should be close.
	 */
	float convertToDegreesMinutes(double deg) const;

	/**
	 * @brief Return the latitude in a GPS-style value DDMM.MMMMM format (degrees * 100 + minutes).
	 *
	 * The result is always positive! To find out if the latitude is south, check location.rawLat().negative
	 * or check the sign of readLatDeg().
	 */
	float readLat(void) const {
		return convertToDegreesMinutes(gpsData.getLocation().lat());
	}

	/**
	 * @brief Return the longitude in a GPS-style value DDMM.MMMMM format (degrees * 100 + minutes).
	 *
	 * The result is always positive! To find out if the latitude is east, check location.rawLng().negative
	 * or check the sign of readLonDeg().
	 */
	float readLon(void) const {
		return convertToDegreesMinutes(gpsData.getLocation().lng());
	}

	/**
	 * @brief Returns the latitude in degrees as a float. May be positive or negative.
	 *
	 * Negative values are used for south latitude.
	 */
	float readLatDeg(void) const {
		return (float) gpsData.getLocation().lat();
	}

	/**
	 * @brief Returns the longitude in degrees as a float. May be positive or negative.
 	 *
	 * Negative values are used for east longitude.
	 */
	float readLonDeg(void) const {
		return (float) gpsData.getLocation().lng();
	}

	/**
	 * @brief Gets the speed in knots
	 */
	float getSpeed() const {
		// The Adafruit library does not check the validity and always returns the last speed
		return (float) gpsData.getSpeed().knots();
	}

	float getAngle() const {
		return (float) gpsData.getCourse().deg();
	}

	uint8_t getHour() const {
		return (uint8_t) gpsData.getTime().hour();
	}

	uint8_t getMinute() const {
		return (uint8_t) gpsData.getTime().minute();
	}

	uint8_t getSeconds() const {
		return (uint8_t) gpsData.getTime().second();
	}

	uint16_t getMilliseconds() const {
		return (uint16_t)gpsData.getTime().centisecond() * 10;
	}

	uint8_t getYear() const {
		return (uint8_t) (gpsData.getDate().year() % 100);
	}

	uint8_t getMonth() const {
		return (uint8_t) gpsData.getDate().month();
	}

	uint8_t getDay() const {
		return (uint8_t) gpsData.getDate().day();
	}

	uint32_t getGpsTimestamp() const {
		TinyGPSTime time = gpsData.getTime();

		// Return timestamp in milliseconds, from last GPS reading
		// 0 if no reading has been done
		// (This returns the milliseconds of current day)
		return time.hour() * 60 * 60 * 1000 + time.minute() * 60 * 1000 + time.second() * 1000 + time.centisecond() * 10;
	}

    uint8_t getFixQuality() const {
    	return gpsData.getLocation().isValid();
    }


	float readHDOP(void) const {
		return (float) gpsData.getHDOP().value() / 100.0;
	}

	float getGpsAccuracy() const {
		// 1.8 taken from specs at https://learn.adafruit.com/adafruit-ultimate-gps/
		return 1.8 * readHDOP();
	}

	float getAltitude() const {
		return gpsData.getAltitude().meters();
	}

	float getGeoIdHeight() const {
		return gpsData.getGeoidSeparation().meters();
	}

	uint8_t getSatellites() const {
		return (uint8_t) gpsData.getSatellites().value();
	}

	bool gpsFix(void) const {
		TinyGPSLocation location = gpsData.getLocation();

		return location.isValid() && location.age() < MAX_GPS_AGE_MS;
	}

	String readLatLon(void) const {
		TinyGPSLocation location = gpsData.getLocation();

	    String latLon = String::format("%lf,%lf", location.lat(), location.lng());
	    return latLon;
	}

	static const unsigned long MAX_GPS_AGE_MS = 10000; // GPS location must be newer than this to be considered valid

	/*

	   char
    checkGPS(void),

	 */

private:
	TinyGPSPlus &gpsData;
};

#endif /* __LEGACYADAPTER_H */
