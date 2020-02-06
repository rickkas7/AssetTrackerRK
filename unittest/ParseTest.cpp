#include "Particle.h"

#include "Adafruit_GPS.h"
#include "TinyGPS++.h"
#include "LegacyAdapter.h"
#include "UbloxGPS.h"

int test1();
int test2();

bool approximatelyEqualFloat(float f1, float f2, float tolerance = 0.0005);

int main(int argc, char *argv[]) {

	int res = test1();
	if (res) {
		return res;
	}
	res = test2();
	if (res) {
		return res;
	}
	return 0;
}


// str must end in \r!
void writeGPS(const char *str, Adafruit_GPS &gpsa, TinyGPSPlus &gpst) {

	for(size_t ii = 0; str[ii]; ii++) {
		gpst.encode(str[ii]);
	}
	gpst.encode('\r');
	gpst.encode('\n');

	// Adafruit GPS takes a non-const pointer and requires a \r terminator (does not include the \n)
	char *mutableCopy = (char *)malloc(strlen(str) + 3);
	strcpy(mutableCopy, "\n");
	strcat(mutableCopy, str);
	strcat(mutableCopy, "\r");

	if (!gpsa.parse(mutableCopy)) {
		// printf("Adafruit failed to parse: %s\n", str);
	}

	free(mutableCopy);
}

// Writes sentence checksum to buf
// buf must be 256 bytes or larger and will be overwritten with a cstring
// str must not contain the $ to start or the * to end or checksum. The start ($), end (*) and checksum will be generated.
// This is used to generate modified test sentences that can still be parsed
void writeSentence(const char *str, char *buf) {
	char *cp = buf;

	uint8_t sum = 0;
	for(size_t ii = 0; str[ii]; ii++) {
		sum ^= (uint8_t)str[ii];
	}

	snprintf(buf, 256, "$%s*%02X", str, sum);
}

bool approximatelyEqualFloat(float f1, float f2, float tolerance) {
	float delta = fabs(f1 - f2);

	return delta < tolerance;
}

int test1() {
	printf("test1 starting\n");

	USARTSerial Serial1;

	// GxRMC:
	// time: UTC hhmmss.ss
	// status: A = valid, V = warning
	// lat: ddmm.mmmmm
	// ns: north/south indicator (N or S)
	// lon: ddmm.mmmmm
	// ew: east/west indicator (E or W)
	// spd: speed over ground (knots)
	// cog: course over ground (degrees)
	// date: ddmmyy
	// rest ignored
#if 1
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		FILE *fd = fopen("t1.txt", "r");
		if (!fd) {
			printf("failed to open t1.txt\n");
			return 1;
		}

		int lineNum = 1;
		while(true) {
			char line[256];

			char *cp = fgets(line, sizeof(line), fd);
			if (!cp) {
				break;
			}

			// Remove line terminator
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = 0;
			}

			writeGPS(line, gpsa, gpst);

			if (!gpst.location.isValid()) {
	//			printf("readLat line=%d TinyGPS not valid %s\n", lineNum, line);
			}

			if (!approximatelyEqualFloat(gpsa.latitude, adapter.readLat(), 0.0005)) {
				printf("t1 readLat line=%d %f != %f %s\n", lineNum, gpsa.latitude, adapter.readLat(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.latitudeDegrees, adapter.readLatDeg(), 0.0005)) {
				printf("t1 readLatDeg line=%d %f != %f %s\n", lineNum, gpsa.latitudeDegrees, adapter.readLatDeg(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.longitude, adapter.readLon(), 0.0005)) {
				printf("t1 readLon line=%d %f != %f %s\n", lineNum, gpsa.longitude, adapter.readLon(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.longitudeDegrees, adapter.readLonDeg(), 0.0005)) {
				printf("t1 readLonDeg line=%d %f != %f %s\n", lineNum, gpsa.longitudeDegrees, adapter.readLonDeg(), line);
				//return 1;
			}

			if (!approximatelyEqualFloat(gpsa.speed, adapter.getSpeed(), 0.05)) {
				printf("t1 getSpeed line=%d %f != %f %s\n", lineNum, gpsa.speed, adapter.getSpeed(), line);
			}

			if (!approximatelyEqualFloat(gpsa.angle, adapter.getAngle(), 0.05)) {
				// The Adafruit library doesn't set the angle if there's an angle but no speed
				if (gpsa.angle != 0) {
					printf("t1 getAngle line=%d %f != %f %s\n", lineNum, gpsa.angle, adapter.getAngle(), line);
				}
			}

			if (gpsa.year != adapter.getYear()) {
				printf("t1 getYear line=%d %d != %d %s\n", lineNum, (int)gpsa.year, (int)adapter.getYear(), line);
			}
			if (gpsa.month != adapter.getMonth()) {
				printf("t1 getMonth line=%d %d != %d %s\n", lineNum, (int)gpsa.month, (int)adapter.getMonth(), line);
			}
			if (gpsa.day != adapter.getDay()) {
				printf("t1 getDay line=%d %d != %d %s\n", lineNum, (int)gpsa.day, (int)adapter.getDay(), line);
			}

			if (gpsa.hour != adapter.getHour()) {
				printf("t1 getHour line=%d %d != %d %s\n", lineNum, (int)gpsa.hour, (int)adapter.getHour(), line);
			}
			if (gpsa.minute != adapter.getMinute()) {
				printf("t1 getMinute line=%d %d != %d %s\n", lineNum, (int)gpsa.minute, (int)adapter.getMinute(), line);
			}
			if (gpsa.seconds != adapter.getSeconds()) {
				printf("t1 getSecond line=%d %d != %d %s\n", lineNum, (int)gpsa.seconds, (int)adapter.getSeconds(), line);
			}

			lineNum++;
		}

		fclose(fd);
	}
#endif

	/*

	const char *sentence;

	sentence = "$GNRMC,143553.00,A,4228.21306,N,07503.88452,W,0.316,,231218,,,A*7E"; // Line 18 in t1.txt
	writeGPS(sentence, gpsa, gpst);
	printf("%s\n", sentence);
	printf("gpsa.latitude=%f latitudeDegrees=%f\n", gpsa.latitude, gpsa.latitudeDegrees);
	printf("gpst.latitude=%f latitudeDegrees=%f raw deg=%d billions=%d\n", adapter.readLat(), adapter.readLatDeg(), gpst.location.rawLat().deg, gpst.location.rawLat().billionths);

	// Adafruit parser has rounding errors on this!
	sentence = "$GNRMC,143554.00,A,4228.21317,N,07503.88439,W,0.438,,231218,,,A*7F"; // Line 19 in t1.txt
	writeGPS(sentence, gpsa, gpst);
	printf("%s\n", sentence);
	printf("gpsa.latitude=%f latitudeDegrees=%f\n", gpsa.latitude, gpsa.latitudeDegrees);
	printf("gpst.latitude=%f latitudeDegrees=%f raw deg=%d billions=%d\n", adapter.readLat(), adapter.readLatDeg(), gpst.location.rawLat().deg, gpst.location.rawLat().billionths);
	 */

#if 0
	char buf[256];
	{
		writeSentence("GNRMC,143553.00,A,4228.21306,S,07503.88452,W,0.316,,231218,,,A", buf);
		printf("%s\n", buf);
		writeSentence("GNRMC,143553.00,A,4228.21306,S,07503.88452,E,0.316,,231218,,,A", buf);
		printf("%s\n", buf);
		writeSentence("GNRMC,143553.00,A,4228.21306,N,07503.88452,E,0.316,,231218,,,A", buf);
		printf("%s\n", buf);
	}
#endif


	// GxGGA: Global positioning system fix data
	// time: UTC hhmmss.ss
	// lat: ddmm.mmmmm
	// ns: north/south indicator (N or S)
	// lon: ddmm.mmmmm
	// ew: east/west indicator (E or W)
	// quality:
	// numSV:
	// HDOP:
	// alt:
	// uALT:
	// sep:
	// uSep:
#if 1
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		FILE *fd = fopen("t2.txt", "r");
		if (!fd) {
			printf("failed to open t2.txt\n");
			return 1;
		}

		int lineNum = 1;
		while(true) {
			char line[256];

			char *cp = fgets(line, sizeof(line), fd);
			if (!cp) {
				break;
			}

			// Remove line terminator
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = 0;
			}

			writeGPS(line, gpsa, gpst);

			if (!gpst.location.isValid()) {
	//			printf("readLat line=%d TinyGPS not valid %s\n", lineNum, line);
			}

			if (!approximatelyEqualFloat(gpsa.latitude, adapter.readLat(), 0.0005)) {
				printf("t2 readLat line=%d %f != %f %s\n", lineNum, gpsa.latitude, adapter.readLat(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.latitudeDegrees, adapter.readLatDeg(), 0.0005)) {
				printf("t2 readLatDeg line=%d %f != %f %s\n", lineNum, gpsa.latitudeDegrees, adapter.readLatDeg(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.longitude, adapter.readLon(), 0.0005)) {
				printf("t2 readLon line=%d %f != %f %s\n", lineNum, gpsa.longitude, adapter.readLon(), line);
				//return 1;

			}

			if (!approximatelyEqualFloat(gpsa.longitudeDegrees, adapter.readLonDeg(), 0.0005)) {
				printf("t2 readLonDeg line=%d %f != %f %s\n", lineNum, gpsa.longitudeDegrees, adapter.readLonDeg(), line);
				//return 1;
			}

			if (!approximatelyEqualFloat(gpsa.HDOP, adapter.readHDOP(), 0.005)) {
				printf("t2 readHDOP line=%d %f != %f %s\n", lineNum, gpsa.HDOP, adapter.readHDOP(), line);
			}

			if (!approximatelyEqualFloat(gpsa.altitude, adapter.getAltitude(), 0.005)) {
				printf("t2 getAltitude line=%d %f != %f %s\n", lineNum, gpsa.altitude, adapter.getAltitude(), line);
			}

			if (!approximatelyEqualFloat(gpsa.geoidheight, adapter.getGeoIdHeight(), 0.005)) {
				printf("t2 getGeoIdHeight line=%d %f != %f %s\n", lineNum, gpsa.geoidheight, adapter.getGeoIdHeight(), line);
			}

			if (gpsa.satellites != adapter.getSatellites()) {
				printf("t2 getSatellites line=%d %d != %d %s\n", lineNum, (int)gpsa.satellites, (int)adapter.getSatellites(), line);
			}


			if (gpsa.year != adapter.getYear()) {
				printf("t2 getYear line=%d %d != %d %s\n", lineNum, (int)gpsa.year, (int)adapter.getYear(), line);
			}
			if (gpsa.month != adapter.getMonth()) {
				printf("t2 getMonth line=%d %d != %d %s\n", lineNum, (int)gpsa.month, (int)adapter.getMonth(), line);
			}
			if (gpsa.day != adapter.getDay()) {
				printf("t2 getDay line=%d %d != %d %s\n", lineNum, (int)gpsa.day, (int)adapter.getDay(), line);
			}

			if (gpsa.hour != adapter.getHour()) {
				printf("t2 getHour line=%d %d != %d %s\n", lineNum, (int)gpsa.hour, (int)adapter.getHour(), line);
			}
			if (gpsa.minute != adapter.getMinute()) {
				printf("t2 getMinute line=%d %d != %d %s\n", lineNum, (int)gpsa.minute, (int)adapter.getMinute(), line);
			}
			if (gpsa.seconds != adapter.getSeconds()) {
				printf("t2 getSecond line=%d %d != %d %s\n", lineNum, (int)gpsa.seconds, (int)adapter.getSeconds(), line);
			}


			lineNum++;
		}

		fclose(fd);
	}
#endif

	// Test fix quality
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		// GPGGA fix testing
		const char *sentence;

		sentence = "$GNGGA,143540.00,,,,,0,03,3.73,,,,,,*4B";
		writeGPS(sentence, gpsa, gpst);
		if (gpsa.fixquality != adapter.getFixQuality()) {
			printf("getFixQuality test 1 %d != %d %s\n", (int)gpsa.fixquality, (int)adapter.getFixQuality(), sentence);
		}

		sentence = "$GNGGA,143541.00,4228.21353,N,07503.88667,W,1,05,1.85,446.4,M,-34.1,M,,*7A";
		writeGPS(sentence, gpsa, gpst);
		if (gpsa.fixquality != adapter.getFixQuality()) {
			printf("getFixQuality test 2 %d != %d %s\n", (int)gpsa.fixquality, (int)adapter.getFixQuality(), sentence);
		}

		sentence = "$GNGGA,143540.00,,,,,0,03,3.73,,,,,,*4B";
		writeGPS(sentence, gpsa, gpst);
		if (gpsa.fixquality != adapter.getFixQuality()) {
			printf("getFixQuality test 3 %d != %d %s\n", (int)gpsa.fixquality, (int)adapter.getFixQuality(), sentence);
		}
	}


#if 1
	// Full sequence AssetTracker v2
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		FILE *fd = fopen("t3.txt", "r");
		if (!fd) {
			printf("failed to open t3.txt\n");
			return 1;
		}

		int lineNum = 1;
		while(true) {
			char line[256];

			char *cp = fgets(line, sizeof(line), fd);
			if (!cp) {
				break;
			}

			// Remove line terminator
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = 0;
			}

			writeGPS(line, gpsa, gpst);

			if (gpsa.fix != adapter.gpsFix()) {
				printf("test line %u failed, lineNum=%d line=%s\n", __LINE__, lineNum, line);
			}

			if (lineNum < 60) {
				if (adapter.gpsFix()) {
					// Lines less than 60 should not have a fix
					printf("test line %u failed, lineNum=%d line=%s\n", __LINE__, lineNum, line);
				}

			}
			else {
				if (!adapter.gpsFix()) {
					// Lines >= 60 should  have a fix
					printf("test line %u failed, lineNum=%d line=%s\n", __LINE__, lineNum, line);
				}

				if (!approximatelyEqualFloat(gpsa.latitudeDegrees, adapter.readLatDeg(), 0.0005)) {
					printf("t3 readLatDeg line=%d %f != %f %s\n", lineNum, gpsa.latitudeDegrees, adapter.readLatDeg(), line);
					//return 1;

				}

				if (!approximatelyEqualFloat(gpsa.longitudeDegrees, adapter.readLonDeg(), 0.0005)) {
					printf("t3 readLonDeg line=%d %f != %f %s\n", lineNum, gpsa.longitudeDegrees, adapter.readLonDeg(), line);
					//return 1;
				}

				if (!approximatelyEqualFloat(gpsa.altitude, adapter.getAltitude(), 0.005)) {
					printf("t3 getAltitude line=%d %f != %f %s\n", lineNum, gpsa.altitude, adapter.getAltitude(), line);
				}
			}

			lineNum++;
		}

		fclose(fd);
	}
#endif

#if 1
	// Full sequence AssetTracker v1
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		FILE *fd = fopen("t4.txt", "r");
		if (!fd) {
			printf("failed to open t4.txt\n");
			return 1;
		}

		int lineNum = 1;
		while(true) {
			char line[256];

			char *cp = fgets(line, sizeof(line), fd);
			if (!cp) {
				break;
			}

			// Remove line terminator
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = 0;
			}

			writeGPS(line, gpsa, gpst);

			// These are a few lines that should have a fix and not and transitioning between fix and non-fix modes
			switch(lineNum) {
			case 3285:
			case 3286:
			case 3882:
			case 3883:
				if (!adapter.gpsFix()) {
					printf("test line %u failed, lineNum=%d adapter.gpsFix=%d line=%s\n", __LINE__, lineNum, (int)adapter.gpsFix(), line);
				}
				break;

			case 3698:
			case 3699:
			case 4189:
			case 4190:
				if (adapter.gpsFix()) {
					printf("test line %u failed, lineNum=%d adapter.gpsFix=%d line=%s\n", __LINE__, lineNum, (int)adapter.gpsFix(), line);
				}
				break;
			}

			/*
			test line 449 failed, lineNum=3285 gpsa.fix=0 adapter.gpsFix=1 line=$GPGGA,144751.000,4226.2528,N,07504.6602,W,1,4,3.56,72.3,M,-33.9,M,,*51
			test line 449 failed, lineNum=3286 gpsa.fix=0 adapter.gpsFix=1 line=$GPGSA,A,3,15,13,10,21,,,,,,,,,3.70,3.56,1.00*03
			test line 449 failed, lineNum=3698 gpsa.fix=1 adapter.gpsFix=0 line=$GPGGA,144921.000,,,,,0,3,,,M,,M,,*40
			test line 449 failed, lineNum=3699 gpsa.fix=1 adapter.gpsFix=0 line=$GPGSA,A,1,,,,,,,,,,,,,,,*1E
			test line 449 failed, lineNum=3882 gpsa.fix=0 adapter.gpsFix=1 line=$GPGGA,145001.000,4225.6845,N,07507.0209,W,1,4,27.24,81.3,M,-33.9,M,,*66
			test line 449 failed, lineNum=3883 gpsa.fix=0 adapter.gpsFix=1 line=$GPGSA,A,3,15,13,10,21,,,,,,,,,32.91,27.24,18.47*36
			test line 449 failed, lineNum=4189 gpsa.fix=1 adapter.gpsFix=0 line=$GPGGA,145108.000,,,,,0,3,,,M,,M,,*42
			test line 449 failed, lineNum=4190 gpsa.fix=1 adapter.gpsFix=0 line=$GPGSA,A,1,,,,,,,,,,,,,,,*1E
			test line 449 failed, lineNum=4419 gpsa.fix=0 adapter.gpsFix=1 line=$GPGGA,145158.000,4224.2243,N,07508.1293,W,1,4,3.30,88.6,M,-33.9,M,,*50
			 */


			if (adapter.gpsFix()) {
				if (!approximatelyEqualFloat(gpsa.latitudeDegrees, adapter.readLatDeg(), 0.0005)) {
					printf("t4 readLatDeg line=%d %f != %f %s\n", lineNum, gpsa.latitudeDegrees, adapter.readLatDeg(), line);
					//return 1;
				}

				if (!approximatelyEqualFloat(gpsa.longitudeDegrees, adapter.readLonDeg(), 0.0005)) {
					printf("t4 readLonDeg line=%d %f != %f %s\n", lineNum, gpsa.longitudeDegrees, adapter.readLonDeg(), line);
					//return 1;
				}

				if (!approximatelyEqualFloat(gpsa.altitude, adapter.getAltitude(), 0.005)) {
					printf("t4 getAltitude line=%d %f != %f %s\n", lineNum, gpsa.altitude, adapter.getAltitude(), line);
				}
			}


			lineNum++;
		}

		fclose(fd);
	}
#endif

#if 1
	// This generates the data to show the error in the degree output from the Adafruit parser caused by the use of
	// float (32-bit) instead of double (64-bit) in the degree-based calculations. Several bits of precision are lost,
	// causing rounding errors.
	{
		Adafruit_GPS gpsa(&Serial1);
		TinyGPSPlus gpst;
		LegacyAdapter adapter(gpst);

		FILE *fd = fopen("t5.txt", "r");
		if (!fd) {
			printf("failed to open t5.txt\n");
			return 1;
		}
		FILE *fdOut = fopen("errorplot/t6.txt", "w");

		int lineNum = 1;
		while(true) {
			char line[256];

			char *cp = fgets(line, sizeof(line), fd);
			if (!cp) {
				break;
			}

			// Remove line terminator
			size_t len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = 0;
			}

			writeGPS(line, gpsa, gpst);

			fprintf(fdOut, "%f,%f,%.8lf,%.8lf\n", gpsa.latitudeDegrees, gpsa.longitudeDegrees, gpst.location.lat(), gpst.location.lng());

			lineNum++;
		}

		fclose(fd);
		fclose(fdOut);

	}
#endif

	// status: A = valid, V = warning
	// spd: speed over ground (knots)
	// cog: course over ground (degrees)
	// date: ddmmyy


	// GxGLL: Latitude and longitude, with time of position fix and status
	// Not parsed by Adafruit or TinyGPS
	// lat: ddmm.mmmmm
	// ns: north/south indicator (N or S)
	// lon: ddmm.mmmmm
	// ew: east/west indicator (E or W)
	// time: UTC hhmmss.ss
	// status: A = valid, V = warning
	// posflag:
	// $GNGLL,4228.21282,N,07503.88480,W,143543.00,A,A*6B

	// GxGSA: GNSS DOP and Active Satellites
	// Not parsed by Adafruit or TinyGPS
	// opMode:
	// navMode:
	// sat number (repeated 12 times)
	// PDOP:
	// HDOP:
	// VDOP:
	// $GNGSA,A,3,15,21,13,24,29,,,,,,,,3.12,1.85,2.52*1D


	// GxGSV: GNSS Satellites in View
	// $GPGSV,2,1,05,13,41,050,32,15,79,055,37,21,67,283,30,24,33,151,37*7D
	// $GPGSV,2,2,05,29,13,203,40*40
	// $GLGSV,1,1,01,,,,35*62
	// numMsg:
	// msgNum:
	// Repeated block:
	//	sv: satellite id
	//	elv: Elevation deg (0-90)
	//	az: Azimuth deg (0-359)
	// 	cno: Signal strength (0-99), blank if not tracking


	printf("test1 completed\n");
	return 0;
}

void compareBinary(const uint8_t *expected, const uint8_t *actual, size_t len, int line) {
	for(size_t ii = 0; ii < len; ii++) {
		if (expected[ii] != actual[ii]) {
			printf("data mismatch ii=%lu expected=%02x actual=%02x line=%d\n", ii, expected[ii], actual[ii], line);
			break;
		}
	}
}

static uint8_t internalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x00,0x00,0xF0,0x7D,0x8A,0x2A};
static uint8_t externalANT[]={0xB5,0x62,0x06,0x13,0x04,0x00,0x01,0x00,0xF0,0x7D,0x8B,0x2E};

int test2() {
	printf("test2 started\n");
	{
		UbloxCommand<16> cmd;

		cmd.setClassId(0x06, 0x13);

		// flags = 0
		// (pinSwitch = false)
		cmd.appendU2(0x0000);

		// pins = 0x7DF0
		// reconfig = false
		// pinOCD = 0b11111 = 31 (not used)
		// pinSCD = 0b01111 = 15 (not used)
		// pinSwitch = 0b10000 = 16
		cmd.appendU2(0x7DF0);

		cmd.updateChecksum();

		if (cmd.getSendLength() != sizeof(internalANT)) {
			printf("wrong length %lu expected %lu line=%d\n", cmd.getSendLength(), sizeof(internalANT), __LINE__);
		}
		compareBinary(internalANT, cmd.getBuffer(), cmd.getSendLength(), __LINE__);
	}

	{
		UbloxCommand<16> cmd;

		cmd.setClassId(0x06, 0x13);

		// flags = 0x0001
		// (pinSwitch = true)
		cmd.appendU2(0x0001);

		// pins = 0x7DF0
		// reconfig = false
		// pinOCD = 0b11111 = 31 (not used)
		// pinSCD = 0b01111 = 15 (not used)
		// pinSwitch = 0b10000 = 16
		cmd.appendU2(0x7DF0);

		cmd.updateChecksum();

		if (cmd.getSendLength() != sizeof(externalANT)) {
			printf("wrong length %lu expected %lu line=%d\n", cmd.getSendLength(), sizeof(externalANT), __LINE__);
		}
		compareBinary(externalANT, cmd.getBuffer(), cmd.getSendLength(), __LINE__);
	}

	printf("test2 completed\n");
	return 0;
}

