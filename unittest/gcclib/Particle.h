// Dummy particle.h file for testing logdata.cpp module from gcc
#ifndef __PARTICLE_H
#define __PARTICLE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cassert>

#include "spark_wiring_string.h"
#include "rng_hal.h"
#include "system_tick_hal.h"

class Stream : public Print {
public:
	virtual ~Stream();

	virtual int available();
	virtual int read();
};

class USARTSerial : public Stream {
public:
	virtual ~USARTSerial();

	void begin(int baud);
	virtual int available();
	virtual int read();
	virtual size_t write(uint8_t c);
};

system_tick_t millis();
void delay(uint32_t ms);

// Arduino compatibility
typedef bool boolean;
typedef uint8_t byte;
#define isAlpha isalpha
#define isDigit isdigit

#define SINGLE_THREADED_SECTION()
#define SINGLE_THREADED_BLOCK()
#define WITH_LOCK(x)
#define TRY_LOCK(x)


#endif /* __PARTICLE_H */
