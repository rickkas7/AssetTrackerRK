/*
TinyGPS++ - a small GPS library for Arduino providing universal NMEA parsing
Based on work by and "distanceBetween" and "courseTo" courtesy of Maarten Lamers.
Suggestion to add satellites, courseTo(), and cardinal() by Matt Monson.
Location precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __TinyGPSPlus_h
#define __TinyGPSPlus_h

#include "Particle.h"
#include <limits.h>
#include <math.h>

#define _GPS_VERSION "0.92" // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945 // miles per hour
#define _GPS_MPS_PER_KNOT 0.51444444 // meters per second
#define _GPS_KMPH_PER_KNOT 1.852 // kilometers per hour
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399
#define _GPS_MAX_FIELD_SIZE 15

// Stuff included in Ardiuno but not Particle:
#ifndef SPARK_WIRING_ARDUINO_CONSTANTS_H
double radians(double deg);
double degrees(double radians);
double sq(double value);
const double TWO_PI = M_PI * 2;
#endif
// End

/**
 * @brief Class to hold raw degree values as integer values instead of floating point
 */
struct RawDegrees
{
	uint16_t deg; 			//<! Degree value (0 <= deg < 365)
	uint32_t billionths; 	//!< Billionths of a segree
	bool negative; 			//!< true if negative (south latitude or east longitude)
public:
	RawDegrees() : deg(0), billionths(0), negative(false)
	{}
};

/**
 * @brief Class to hold a location value
 */
struct TinyGPSLocation
{
	friend class TinyGPSPlus;
public:
	/**
	 * @brief Returns true if the location is valid
	 *
	 * The valid flag will be false if the GPS loses its fix, but will not be invalidated if the GPS
	 * stops returning data. If that is a possibility, you should also check the age.
	 */
	bool isValid() const    { return valid; }

	/**
	 * @brief Returns true if the value has been updated since last read
	 *
	 * The updated flag is set to false after reading data (lag, lng, rawLat, rawLng) and is set to
	 * true when a valid location is received from the GPS.
	 */
	bool isUpdated() const  { return updated; }

	/**
	 * @brief Returns the age of the location value in milliseconds
	 *
	 * If the value has never been set, then ULONG_MAX is returned.
	 *
	 * You might check to see if age is < 10000 to make sure the value has been retrieved in the
	 * last 10 seconds, for example.
	 */
	uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

	/**
	 * @brief Returns the raw latitude.
	 *
	 * The raw latitude separates out the whole part (in degrees), billionths of a degree, and
	 * sign, instead of using a double floating point number.
	 *
	 * This method is not const because it clears the updated flag.
	 */
	const RawDegrees &rawLat()     { updated = false; return rawLatData; }

	/**
	 * @brief Returns the raw longitude.
	 *
	 * The raw latitude separates out the whole part (in degrees), billionths of a degree, and
	 * sign, instead of using a double floating point number.
	 *
	 * This method is not const because it clears the updated flag.
	 */
	const RawDegrees &rawLng()     { updated = false; return rawLngData; }

	/**
	 * @brief Returns the latitude in degrees as a signed double floating point value.
	 *
	 * Positive values are for north latitude. Negative values are for south latitude.
	 * Valid values are 0 <= lat() < 360.0
	 *
	 * This method is not const because it clears the updated flag.
	 */
	double lat();

	/**
	 * @brief Returns the longitude in degrees as a signed double floating point value.
	 *
	 * Positive values are for west longitude. Negative values are for east longitude.
	 * Valid values are 0 <= lng() < 360.0
	 *
	 * This method is not const because it clears the updated flag.
	 */
	double lng();

	/**
	 * @brief Invalidates the valid flag
	 *
	 * This is used internally when we get a valid GPS sentence that does not have a fix.
	 * It can be checked by using the isValid() method.
	 */
	void invalidate() { valid = false; }

	TinyGPSLocation() : valid(false), updated(false), lastCommitTime(0)
	{}

private:
	bool valid, updated;
	RawDegrees rawLatData, rawLngData, rawNewLatData, rawNewLngData;
	uint32_t lastCommitTime;
	void commit();
	void setLatitude(const char *term);
	void setLongitude(const char *term);
};

struct TinyGPSDate
{
	friend class TinyGPSPlus;
public:
	/**
	 * @brief Returns true if the date is valid
	 */
	bool isValid() const       { return valid; }
	bool isUpdated() const     { return updated; }
	uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

	uint32_t value()           { updated = false; return date; }
	uint16_t year();
	uint8_t month();
	uint8_t day();

	TinyGPSDate() : valid(false), updated(false), date(0), newDate(0), lastCommitTime(0)
	{}

private:
	bool valid, updated;
	uint32_t date, newDate;
	uint32_t lastCommitTime;
	void commit();
	void setDate(const char *term);
};

struct TinyGPSTime
{
	friend class TinyGPSPlus;
public:
	/**
	 * @brief Returns true if the time is valid
	 */
	bool isValid() const       { return valid; }
	bool isUpdated() const     { return updated; }
	uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }

	uint32_t value()           { updated = false; return time; }
	uint8_t hour();
	uint8_t minute();
	uint8_t second();
	uint8_t centisecond();

	TinyGPSTime() : valid(false), updated(false), time(0), newTime(0), lastCommitTime(0)
	{}

private:
	bool valid, updated;
	uint32_t time, newTime;
	uint32_t lastCommitTime;
	void commit();
	void setTime(const char *term);
};

struct TinyGPSDecimal
{
	friend class TinyGPSPlus;
public:
	/**
	 * @brief Returns true if the value is valid
	 */
	bool isValid() const    { return valid; }
	bool isUpdated() const  { return updated; }
	uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
	int32_t value()         { updated = false; return val; }
	void invalidate() { valid = false; }

	TinyGPSDecimal() : valid(false), updated(false), lastCommitTime(0), val(0), newval(0)
	{}

private:
	bool valid, updated;
	uint32_t lastCommitTime;
	int32_t val, newval;
	void commit();
	void set(const char *term);
};

struct TinyGPSInteger
{
	friend class TinyGPSPlus;
public:
	/**
	 * @brief Returns true if the value is valid
	 */
	bool isValid() const    { return valid; }
	bool isUpdated() const  { return updated; }
	uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
	uint32_t value()        { updated = false; return val; }
	void invalidate() { valid = false; }

	TinyGPSInteger() : valid(false), updated(false), lastCommitTime(0), val(0), newval(0)
	{}

private:
	bool valid, updated;
	uint32_t lastCommitTime;
	uint32_t val, newval;
	void commit();
	void set(const char *term);
};

struct TinyGPSSpeed : TinyGPSDecimal
{
	/**
	 * @brief Returns the speed in knots
	 */
	double knots()    { return value() / 100.0; }

	/**
	 * @brief Returns the speed in miles per hour
	 */
	double mph()      { return _GPS_MPH_PER_KNOT * value() / 100.0; }

	/**
	 * @brief Returns the speed in meters per second
	 */
	double mps()      { return _GPS_MPS_PER_KNOT * value() / 100.0; }

	/**
	 * @brief Returns the speed in kilometers per hour
	 */
	double kmph()     { return _GPS_KMPH_PER_KNOT * value() / 100.0; }
};

struct TinyGPSCourse : public TinyGPSDecimal
{
	/**
	 * @brief Returns the course (direction you are heading) in degrees
	 *
	 * 0 <= deg < 360
	 */
	double deg()      { return value() / 100.0; }
};

struct TinyGPSAltitude : TinyGPSDecimal
{
	/**
	 * @brief Returns the altitude in meters as a double floating point value.
	 */
	double meters()       { return value() / 100.0; }

	/**
	 * @brief Returns the altitude in miles as a double floating point value.
	 */
	double miles()        { return _GPS_MILES_PER_METER * value() / 100.0; }

	/**
	 * @brief Returns the altitude in kilometers as a double floating point value.
	 */
	double kilometers()   { return _GPS_KM_PER_METER * value() / 100.0; }

	/**
	 * @brief Returns the altitude in feet as a double floating point value.
	 */
	double feet()         { return _GPS_FEET_PER_METER * value() / 100.0; }
};

class TinyGPSPlus; // Forward declaration

class TinyGPSCustom
{
public:
	TinyGPSCustom() : lastCommitTime(0), valid(false), updated(false), sentenceName(0), termNumber(0), next(0) {};
	TinyGPSCustom(TinyGPSPlus &gps, const char *sentenceName, int termNumber);
	void begin(TinyGPSPlus &gps, const char *_sentenceName, int _termNumber);

	bool isUpdated() const  { return updated; }
	bool isValid() const    { return valid; }
	uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX; }
	const char *value()     { updated = false; return buffer; }

private:
	void commit();
	void set(const char *term);

	char stagingBuffer[_GPS_MAX_FIELD_SIZE + 1];
	char buffer[_GPS_MAX_FIELD_SIZE + 1];
	unsigned long lastCommitTime;
	bool valid, updated;
	const char *sentenceName;
	int termNumber;
	friend class TinyGPSPlus;
	TinyGPSCustom *next;
};

/**
 * @brief Container to hold the data values.
 *
 * The TinyGPSPlus object has this as a public superclass so you can still access the location field,
 * for example. However, it also means you can copy all of the values to a local variable if you
 * want to.
 *
 * Also, it's used internally so a temporary copy can be updated while parsing the GPS data. If the
 * checksum is valid, then the data will be copied into the TinyGPSPlus superclass TinyGPSData. This
 * is necessary to avoid reading half-written values or blocking for long periods of time when
 * supporting multi-threaded mode.
 *
 * For best thread safety, you should avoid using fields like location directly and instead should
 * use methods like getLocation() to make a copy of the location data. This will assure that your
 * data is valid and does not change while you are reading it.
 */
class TinyGPSData {
public:
	TinyGPSLocation location;
	TinyGPSDate date;
	TinyGPSTime time;
	TinyGPSSpeed speed;
	TinyGPSCourse course;
	TinyGPSAltitude altitude;
	TinyGPSAltitude geoidSeparation;
	TinyGPSInteger satellites;
	TinyGPSDecimal hdop;

	TinyGPSLocation getLocation() const {
	    SINGLE_THREADED_BLOCK() {
	    	return location;
	    }
	}
	TinyGPSDate getDate() const {
	    SINGLE_THREADED_BLOCK() {
	    	return date;
	    }
	}
	TinyGPSTime getTime() const {
	    SINGLE_THREADED_BLOCK() {
	    	return time;
	    }
	}
	TinyGPSSpeed getSpeed() const {
	    SINGLE_THREADED_BLOCK() {
	    	return speed;
	    }
	}
	TinyGPSCourse getCourse() const {
	    SINGLE_THREADED_BLOCK() {
	    	return course;
	    }
	}
	TinyGPSAltitude getAltitude() const {
	    SINGLE_THREADED_BLOCK() {
	    	return altitude;
	    }
	}
	TinyGPSAltitude getGeoidSeparation() const {
	    SINGLE_THREADED_BLOCK() {
	    	return geoidSeparation;
	    }
	}
	TinyGPSInteger getSatellites() const {
	    SINGLE_THREADED_BLOCK() {
	    	return satellites;
	    }
	}
	TinyGPSDecimal getHDOP() const {
	    SINGLE_THREADED_BLOCK() {
	    	return hdop;
	    }
	}
	void copyDataTo(TinyGPSData &other) const{
	    SINGLE_THREADED_BLOCK() {
	    	other.operator=(*this);
	    }
	}

};

class TinyGPSPlus : public TinyGPSData
{
public:
	TinyGPSPlus();
	bool encode(char c); // process one character received from GPS
	TinyGPSPlus &operator << (char c) {encode(c); return *this;}

	static const char *libraryVersion() { return _GPS_VERSION; }

	static double distanceBetween(double lat1, double long1, double lat2, double long2);
	static double courseTo(double lat1, double long1, double lat2, double long2);
	static const char *cardinal(double course);

	static int32_t parseDecimal(const char *term);
	static void parseDegrees(const char *term, RawDegrees &deg);

	uint32_t charsProcessed()   const { return encodedCharCount; }
	uint32_t sentencesWithFix() const { return sentencesWithFixCount; }
	uint32_t failedChecksum()   const { return failedChecksumCount; }
	uint32_t passedChecksum()   const { return passedChecksumCount; }

private:
	enum {GPS_SENTENCE_GPGGA, GPS_SENTENCE_GPRMC, GPS_SENTENCE_OTHER};

	// temporary data until we get a valid checksum. This is also used to avoid
	// corrupted data in multithreaded mode
	TinyGPSData tempData;

	// parsing state variables
	uint8_t parity;
	bool isChecksumTerm;
	char term[_GPS_MAX_FIELD_SIZE];
	uint8_t curSentenceType;
	uint8_t curTermNumber;
	uint8_t curTermOffset;
	bool sentenceHasFix;

	// custom element support
	friend class TinyGPSCustom;
	TinyGPSCustom *customElts;
	TinyGPSCustom *customCandidates;
	void insertCustom(TinyGPSCustom *pElt, const char *sentenceName, int index);

	// statistics
	uint32_t encodedCharCount;
	uint32_t sentencesWithFixCount;
	uint32_t failedChecksumCount;
	uint32_t passedChecksumCount;

	// internal utilities
	int fromHex(char a);
	bool endOfTermHandler();
};

#endif // def(__TinyGPSPlus_h)
