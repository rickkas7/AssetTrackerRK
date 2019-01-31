

all : ParseTest
	./ParseTest

ParseTest : ParseTest.cpp ../src/TinyGPS++.cpp ../src/TinyGPS++.h ../src/LegacyAdapter.cpp ../src/LegacyAdapter.h Adafruit_GPS.cpp Adafruit_GPS.h  libwiringgcc
	gcc ParseTest.cpp ../src/TinyGPS++.cpp ../src/LegacyAdapter.cpp Adafruit_GPS.cpp gcclib/libwiringgcc.a -std=c++11 -lc++ -Igcclib -I../src -DPARTICLE -o ParseTest

check : ParseTest.cpp ../src/TinyGPS++.cpp ../src/TinyGPS++.h ../src/LegacyAdapter.cpp ../src/LegacyAdapter.h Adafruit_GPS.cpp Adafruit_GPS.h libwiringgcc
	gcc ParseTest.cpp ../src/TinyGPS++.cpp ../src/LegacyAdapter.cpp Adafruit_GPS.cpp gcclib/libwiringgcc.a -g -O0 -std=c++11 -lc++ -Igcclib -I ../src -DPARTICLE -o ParseTest && valgrind --leak-check=yes ./ParseTest 

libwiringgcc :
	cd gcclib && make libwiringgcc.a 	
	
.PHONY: libwiringgcc