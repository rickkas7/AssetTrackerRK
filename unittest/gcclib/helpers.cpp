#include "Particle.h"

#include <sys/time.h>

extern "C"
char *itoa ( int value, char * str, int base ) {

	if (base == 16) {
		sprintf(str, "%x", value);
	}
	else
	if (base == 8) {
		sprintf(str, "%o", value);
	}
	else {
		sprintf(str, "%d", value);
	}

	return str;
}

extern "C"
char *utoa ( unsigned int value, char * str, int base ) {

	if (base == 16) {
		sprintf(str, "%x", value);
	}
	else
	if (base == 8) {
		sprintf(str, "%o", value);
	}
	else {
		sprintf(str, "%u", value);
	}

	return str;
}

extern "C"
char *ltoa (unsigned long value, char * str, int base ) {

	if (base == 16) {
		sprintf(str, "%lx", value);
	}
	else
	if (base == 8) {
		sprintf(str, "%lo", value);
	}
	else {
		sprintf(str, "%ld", value);
	}

	return str;
}

extern "C"
char *ultoa (unsigned long value, char * str, int base ) {

	if (base == 16) {
		sprintf(str, "%lx", value);
	}
	else
	if (base == 8) {
		sprintf(str, "%lo", value);
	}
	else {
		sprintf(str, "%lu", value);
	}

	return str;
}

extern "C"
uint32_t HAL_RNG_GetRandomNumber(void) {
	// This isn't right, there should be a cryptographically sound random number here,
	// but for testing this will be fine.
	return (uint32_t) rand();
}

system_tick_t millis() {
	static time_t start = 0;

	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (start == 0) {
		start = tv.tv_sec;
	}

	return (tv.tv_sec - start) * 1000 + (tv.tv_usec / 1000);
}

void delay(uint32_t ms) {
	struct timeval tvStart;
	struct timeval tvCur;

	gettimeofday(&tvStart, NULL);

	while(true) {
		gettimeofday(&tvCur, NULL);

		uint32_t cur = (tvCur.tv_sec - tvStart.tv_sec) * 1000 + ((tvCur.tv_usec - tvStart.tv_usec) / 1000);

		if (cur >= ms) {
			break;
		}
	}
}


Stream::~Stream() {
}

int Stream::available() {
	return 0;
}
int Stream::read() {
	return -1;
}

USARTSerial::~USARTSerial() {
}

void USARTSerial::begin(int baud) {

}
int USARTSerial::available() {
	return 0;
}
int USARTSerial::read() {
	return -1;
}
size_t USARTSerial::write(uint8_t c) {
	return 0;
}





