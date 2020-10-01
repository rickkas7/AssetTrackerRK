
#include "AssetTrackerRK.h"

SYSTEM_THREAD(ENABLED);

SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;

void displayInfo(); // forward declaration

const unsigned long PUBLISH_PERIOD = 120000;
const unsigned long SERIAL_PERIOD = 5000;

AssetTrackerFeather6 t;
unsigned long lastSerial = 0;
unsigned long lastPublish = 0;
unsigned long startFix = 0;
bool gettingFix = false;

void setup()
{
	t.setup();

    startFix = millis();
    gettingFix = true;
}

void loop()
{
    t.loop();

	displayInfo();
}

void displayInfo()
{
	if (millis() - lastSerial >= SERIAL_PERIOD) {
		lastSerial = millis();

		char buf[128];
		if (t.gpsFix()) {
			snprintf(buf, sizeof(buf), "location:%f,%f altitude:%f satellites:%d hdop:%d", t.readLatDeg(), t.readLonDeg(), t.getAltitude(), t.getSatellites(), t.getTinyGPSPlus()->getHDOP().value());
			if (gettingFix) {
				gettingFix = false;
				unsigned long elapsed = millis() - startFix;
				Log.info("%lu milliseconds to get GPS fix", elapsed);
			}
		}
		else {
			snprintf(buf, sizeof(buf), "no location satellites:%d", t.getSatellites());
			if (!gettingFix) {
				gettingFix = true;
				startFix = millis();
			}
		}
		Log.info(buf);

		if (Particle.connected()) {
			if (millis() - lastPublish >= PUBLISH_PERIOD) {
				lastPublish = millis();
				Particle.publish("gps", buf, PRIVATE);
			}
		}

        t.getValue(0x0A, 0x09, [](UbloxCommandBase *cmd) {
            Log.info("MON-HW len=%u", cmd->getPayloadLen());
            Log.dump(cmd->getData(), 60);
            Log.print("\n");
        });


	}

}
