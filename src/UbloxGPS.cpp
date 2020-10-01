
#include "UbloxGPS.h"

#include "AssetTrackerRK.h"

// Define this for regular logging (comment out to save code/string space, about 682 bytes).
// Note: You can also just turn off app.ublox messages in your log handler. It won't save code
// space but it will reduce the stuff in your logs.
#define UBLOX_DEBUG_ENABLE

// Define this for more verbose debugging logs. Saves about 528 bytes of code flash space.
// #define UBLOX_DEBUG_VERBOSE_ENABLE

// Dump out the information about the AssistNow packets being downloaded. Usually only needed when
// debugging AssistNow. It enables quite a bit of code and a lot of strings, so turning this off will
// save some code flash space (about 688 bytes).
// #define ASSISTNOW_DEBUG_ENABLE


#if defined(UBLOX_DEBUG_ENABLE) || defined(UBLOX_DEBUG_VERBOSE_ENABLE) || defined(ASSISTNOW_DEBUG_ENABLE)
static Logger _log("app.ublox");
#endif 


#ifdef UBLOX_DEBUG_ENABLE
#define UBLOX_DEBUG(x) _log.info x
#else
#define UBLOX_DEBUG(x)
#endif

#ifdef UBLOX_DEBUG_VERBOSE_ENABLE
#define UBLOX_DEBUG_VERBOSE(x) _log.info x
#else
#define UBLOX_DEBUG_VERBOSE(x)
#endif



UbloxCommandBase::UbloxCommandBase(uint8_t *buffer, size_t bufferSize) : buffer(buffer), bufferSize(bufferSize) {
}

UbloxCommandBase::UbloxCommandBase(size_t bufferSize) : bufferSize(bufferSize) {
	buffer = new uint8_t[bufferSize];
	if (buffer) {
		deleteBuffer = true;
	}
}

UbloxCommandBase::~UbloxCommandBase() {
	if (deleteBuffer) {
		delete[] buffer;
		buffer = 0;
	}
}

bool UbloxCommandBase::decode(char ch) {
	switch(state) {
	case State::LOOKING_FOR_START:
		if ((uint8_t)ch == SYNC_1) {
			bufferOffset = 0;
			buffer[bufferOffset++] = (uint8_t) ch;
			state = State::LOOKING_FOR_LENGTH;
		}
		break;

	case State::LOOKING_FOR_LENGTH:
		// 0 SYNC_1
		// 1 SYNC_2
		// 2 CLASS
		// 3 ID
		// 4 - 5 Length (Little Endian, LSB first)
		// 6 - ?? Data
		// ?? CHK_A
		// ?? CHK_B
		buffer[bufferOffset++] = (uint8_t) ch;

		// HEADER_PLUS_CRC_LEN = 8: SYNC_1, SYNC_2, CLASS, ID, Length (2 bytes), CHK_A, CHK_B
		if (bufferOffset >= HEADER_PLUS_CRC_LEN) {
			// Minimum message is 8 bytes with an empty payload
			if (buffer[1] != SYNC_2) {
				//Log.info("missing SYNC_2");
				discardToNextSync1();
				break;
			}
			// Length
			payloadLen = ((uint16_t)buffer[5] << 8) | buffer[4];

			if ((payloadLen + HEADER_PLUS_CRC_LEN) > bufferSize) {
				// Payload is too large, discard the whole thing
				// Or, the payload length is not valid, which is why we don't skip that many bytes.
				UBLOX_DEBUG(("data too long payloadLen=%u", payloadLen));
				discardToNextSync1();
				break;
			}
			state = State::LOOKING_FOR_MESSAGE;
		}
		break;

	case State::LOOKING_FOR_MESSAGE:
		buffer[bufferOffset++] = (uint8_t) ch;

		if (bufferOffset >= (payloadLen + HEADER_PLUS_CRC_LEN)) {
			// Have the entire message and CRC. Validate the CRC.

			uint8_t ckA, ckB;

			calculateChecksum(ckA, ckB);

			size_t crcOffset = CRC_START_OFFSET + CRC_HEADER_LEN + payloadLen;
			if (buffer[crcOffset] != ckA || buffer[crcOffset + 1] != ckB) {
				UBLOX_DEBUG(("invalid CRC"));
				discardToNextSync1();
				break;
			}

#ifdef UBLOX_DEBUG_VERBOSE_ENABLE
			if (getMsgClass() == 0x05) {
				uint8_t clsID = getU1(0);
				uint8_t msgID = getU1(1);

				UBLOX_DEBUG_VERBOSE(("%s clsID=%02x msgID=%02x", ((getMsgId() == 0) ? "NACK" : "ACK"), clsID, msgID));
			}
			else {
				UBLOX_DEBUG_VERBOSE(("got message msgClass=%02x msgId=%02x payloadLen=%u", getMsgClass(), getMsgId(), getPayloadLen()));
			}
#endif // UBLOX_DEBUG_VERBOSE_ENABLE

			// Got a valid message, call registered message handlers
			callHandlers();

			// Discard data and search for sync again
			bufferOffset = 0;
			state = State::LOOKING_FOR_START;
		}
		break;
	}

	return false;
}

void UbloxCommandBase::updateChecksum() {
	buffer[0] = SYNC_1;
	buffer[1] = SYNC_2;

	buffer[4] = (uint8_t) payloadLen;
	buffer[5] = (uint8_t) (payloadLen >> 8);


	size_t crcOffset = CRC_START_OFFSET + CRC_HEADER_LEN + payloadLen;

	calculateChecksum(buffer[crcOffset], buffer[crcOffset + 1]);
}

void UbloxCommandBase::calculateChecksum(uint8_t &ckA, uint8_t &ckB) const {
	ckA = 0;
	ckB = 0;

	for(size_t ii = CRC_START_OFFSET; ii < (CRC_START_OFFSET + payloadLen + CRC_HEADER_LEN); ii++) {
		ckA += buffer[ii];
		ckB += ckA;
	}
}

void UbloxCommandBase::addHandler(UbloxMessageHandler *handler) {
	handlers.push_back(handler);
}

void UbloxCommandBase::removeHandler(UbloxMessageHandler *handler) {

	auto it = handlers.begin();

	while(it != handlers.end()) {
		if (*it == handler) {
			it = handlers.erase(it);
		}
		else {
			it++;
		}
	}
}

void UbloxCommandBase::callHandlers() {
	for(auto it = handlers.begin(); it != handlers.end(); ) {
		auto handler = *it;
		bool increment = true;

		if (handler->classFilter == 0xff || handler->classFilter == getMsgClass()) {
			if (handler->idFilter == 0xff || handler->idFilter == getMsgId()) {
				handler->handler(this);
				if (handler->removeAndDelete) {
					it = handlers.erase(it);
					delete handler;
					increment = false;
				}
			}
		}
		if (increment) {
			it++;
		}
	}
}

void UbloxCommandBase::setClassId(uint8_t msgClass, uint8_t msgId) {
	buffer[CLASS_OFFSET] = msgClass;
	buffer[ID_OFFSET] = msgId;
}


bool UbloxCommandBase::getData(size_t offset, void *data, size_t dataLen) const {
	if ((offset + dataLen) <= payloadLen) {
		memcpy(data, &buffer[DATA_OFFSET + offset], dataLen);
		return true;
	}
	else {
		return false;
	}
}


bool UbloxCommandBase::setData(size_t offset, const void *data, size_t dataLen) {
	if ((offset + dataLen) <= (bufferSize - HEADER_PLUS_CRC_LEN)) {
		if ((offset + dataLen) > payloadLen) {
			payloadLen = offset + dataLen;
		}
		memcpy(&buffer[DATA_OFFSET + offset], data, dataLen);
		return true;
	}
	else {
		return false;
	}
}

bool UbloxCommandBase::appendData(const void *data, size_t dataLen) {
	if ((payloadLen + dataLen) <= (bufferSize - HEADER_PLUS_CRC_LEN)) {
		memcpy(&buffer[DATA_OFFSET + payloadLen], data, dataLen);
		payloadLen += dataLen;
		return true;
	}
	else {
		return false;
	}
}

bool UbloxCommandBase::fillData(uint8_t value, size_t dataLen) {
	if ((payloadLen + dataLen) <= (bufferSize - HEADER_PLUS_CRC_LEN)) {
		memset(&buffer[DATA_OFFSET + payloadLen], value, dataLen);
		payloadLen += dataLen;
		return true;
	}
	else {
		return false;
	}
}


uint8_t UbloxCommandBase::getU1(size_t offset) const {
	uint8_t result = 0;
	getData(offset, &result, sizeof(result));
	return result;
}

uint16_t UbloxCommandBase::getU2(size_t offset) const {
	uint16_t result = 0;
	getData(offset, &result, sizeof(result));
	return result;
}


uint32_t UbloxCommandBase::getU4(size_t offset) const {
	uint32_t result = 0;
	getData(offset, &result, sizeof(result));
	return result;
}

float UbloxCommandBase::getR4(size_t offset) const {
	float result = 0;
	getData(offset, &result, sizeof(result));
	return result;
}

double UbloxCommandBase::getR8(size_t offset) const {
	double result = 0;
	getData(offset, &result, sizeof(result));
	return result;
}


void UbloxCommandBase::discardToNextSync1() {
	for(size_t ii = 1; ii < bufferOffset; ii++) {
		if (buffer[ii] == SYNC_1) {
			// Found a 0xb5, move that to the beginning of the buffer
			memmove(buffer, &buffer[ii], bufferOffset - ii);
			bufferOffset -= ii;
			state = State::LOOKING_FOR_MESSAGE;
			return;
		}
	}
	// No SYNC_1, so go into LOOKING_FOR_START
	bufferOffset = 0;
	state = State::LOOKING_FOR_START;
}

UbloxCommandBase *UbloxCommandBase::clone() {
	UbloxCommandBase *copy = new UbloxCommandBase(payloadLen);
	if (copy) {
		if (copy->buffer) {
			memcpy(copy->buffer, buffer, payloadLen);
			copy->bufferOffset = bufferOffset;
			return copy;
		}
		else {
			delete copy;
			return NULL;
		}
	}
	else {
		return NULL;
	}
}


Ublox *Ublox::instance = 0;


Ublox::Ublox() {	
	instance = this;
}

Ublox::~Ublox() {
}

void Ublox::setup() {
	AssetTrackerBase::getInstance()->setExternalDecoder([this](char ch) {
		return decode(ch);
	});
}

void Ublox::loop() {
	
}


void Ublox::getValue(uint8_t msgClass, uint8_t msgId, std::function<void(UbloxCommandBase *)> callback) {

	// This is not currently used and not well tested yet, beware!

	UbloxMessageHandler *handler = new UbloxMessageHandler();

	handler->classFilter = msgClass;
	handler->idFilter = msgId;
	handler->handler = [this,handler,callback](UbloxCommandBase *cmd) {
		callback(cmd);
		handler->removeAndDelete = true;
	};

	addHandler(handler);

	UbloxCommand<0> cmd;

	cmd.setClassId(msgClass, msgId);
	sendCommand(&cmd);
}


void Ublox::sendCommand(UbloxCommandBase *cmd) {
	cmd->updateChecksum();

	AssetTrackerBase::getInstance()->sendCommand(cmd->getBuffer(), cmd->getSendLength());
}



void Ublox::setAntenna(bool external) {
	UbloxCommand<4> cmd;

	cmd.setClassId(0x06, 0x13); // CFG-ANT

	// svcs bit: 0 = internal, 1 = external
	cmd.appendU2(external ? 0x0001 : 0x0000);

	// pins: 0x7DF0 = 0111 1101 1111 0000  
	// Reconfig = false
	// pinOCD = 0b11111 = 0x1f
	// pinSCD = 0b01111 = 0x0f
	// pinSwitch = 0b10000 = 0x10
	cmd.appendU2(0x7DF0);

	sendCommand(&cmd);
}

void Ublox::resetReceiver(StartType startType, ResetMode resetMode) {
	UbloxCommand<5> cmd;

	cmd.setClassId(0x06, 0x04); // CFG-RST
	cmd.appendU2((uint16_t)startType);
	cmd.appendU2((uint16_t)resetMode);
	cmd.appendU1(0);

	sendCommand(&cmd);
}

void Ublox::enableAckAiding() {

	UbloxCommand<40> cmd;

	// CFG-NAVX5 0x06 0x23
	// Navigation Engine Expert Settings
	cmd.setClassId(0x06, 0x23); 
	cmd.fillData(0, 40);

	cmd.setU2(0, 0x0002); // version 2
	cmd.setU2(2, 0x0400); // ackAid

	sendCommand(&cmd);


}

void Ublox::enableExtIntBackup(bool enable) {

	getValue(0x06, 0x3B, [this, enable](UbloxCommandBase *cmd) {
		// Clear: 
		//	extintSel (0x10) - use EXTINT0
		//	extintWake (0x20) - disable
		// Set to enable
		// 	extintBackup (0x40) - force receiver into BACKUP mode when selected EXTINT pin is 'low'

		uint32_t value = cmd->getU4(0x04); // Flags
		value &= ~0x00000070;
		if (enable) {
			value |= 0x00000040;
		}
		cmd->setU1(0x04, value);
		
		sendCommand(cmd);
	});

}

void Ublox::disableTimePulse() {
	// CFG-TP5
	getValue(0x06, 0x31, [this](UbloxCommandBase *cmd) {
		uint32_t flags = cmd->getU4(28);

		flags &= ~0x0001;

		cmd->setU4(28, flags);

		sendCommand(cmd);

		updateMON_HW([](UbloxCommandBase *cmd) {
			// Set VP entry 0x0b (TIMEPULSE) to PIO (0xff)
			cmd->setU1(28 + 0x0b, 0xff);
			return true;
		});
	});
}

void Ublox::setPIO(uint32_t andMaskSel, uint32_t orMaskSel, uint32_t andMaskDir, uint32_t orMaskDir, uint32_t andMaskVal, uint32_t orMaskVal) {

	updateMON_HW([andMaskSel, orMaskSel, andMaskDir, orMaskDir, andMaskVal, orMaskVal](UbloxCommandBase *cmd) {
		uint32_t value = cmd->getU4(0); // pinSel
		value &= andMaskSel;
		value |= orMaskSel;
		Log.info("pinSel=%08lx", value);
		cmd->setU4(0, value);
		
		value = cmd->getU4(8); // pinDir
		value &= andMaskDir;
		value |= orMaskDir;
		Log.info("pinDir=%08lx", value);
		cmd->setU4(8, value);

		value = cmd->getU4(12); // pinVal
		value &= andMaskVal;
		value |= orMaskVal;
		Log.info("pinVal=%08lx", value);
		cmd->setU4(12, value);

		return true; 
	});

}

void Ublox::updateMON_HW(std::function<bool(UbloxCommandBase *cmd)> updateFn) {
	// MON-HW
	getValue(0x0a, 0x09, [this, updateFn](UbloxCommandBase *cmd) {
		
		if (updateFn(cmd)) {
			// updateFn returned true, so send the modified command
			sendCommand(cmd);
		}
	});

}



UbloxAssistNow *UbloxAssistNow::instance = 0;
static const char *ASSIST_NOW_EVENT_NAME = "AssistNow";

UbloxAssistNow::UbloxAssistNow() {
	instance = this;
}

UbloxAssistNow::~UbloxAssistNow() {

}

void UbloxAssistNow::setup() {
}

void UbloxAssistNow::loop() {
	if (stateHandler) {
		stateHandler(this);
	}	
	if (download) {
		download->locator.loop();
	}
}

void UbloxAssistNow::stateWaitConnected() {
	if (!Particle.connected()) {
		return;
	}

	// See if we have a GPS fix. If we
	if (AssetTrackerBase::getInstance()->gpsFix()) {
		UBLOX_DEBUG(("Already have GPS fix, skipping AssistNow"));
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}

	if (assistNowKey.length() == 0) {
		UBLOX_DEBUG(("No key, can't use AssistNow"));
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}

	if (!download) {
		download = new AssistNowDownload();

		// When we support other GNSS like BeiDou and Galileo, need to figure out the correct buffer sizes
		size_t bufSize;
		if (disableLocation) {
			bufSize = 4500;
		}
		else {
			bufSize = 3000;
		}

		if (!download || !download->alloc(bufSize)) {
			UBLOX_DEBUG(("failed to allocate AssistNowDownload"));
			stateHandler = &UbloxAssistNow::stateDone;
			return;
		}
	}
	
	if (!disableLocation) {
		snprintf((char *)download->buffer, download->bufferSize, "hook-response/%s/%s", ASSIST_NOW_EVENT_NAME, System.deviceID().c_str());
		Particle.subscribe((char *)download->buffer, &UbloxAssistNow::subscriptionHandler, this, MY_DEVICES);

		download->locator.withEventName(ASSIST_NOW_EVENT_NAME);
		download->locator.publishLocation();
		stateHandler = &UbloxAssistNow::stateWaitLocation;
		stateTime = millis();
	}
	else {
		stateHandler = &UbloxAssistNow::stateSendRequest;
	}
}

void UbloxAssistNow::stateWaitLocation() {
	if (millis() - stateTime >= waitLocationTimeoutMs) {
		UBLOX_DEBUG(("Timed out getting location information, defaulting to no location mode"));
		disableLocation = true;
		stateHandler = &UbloxAssistNow::stateSendRequest;
	}

}

void UbloxAssistNow::stateSendRequest() {

	if (download->client.connect(assistNowServer, 80)) {
		UBLOX_DEBUG(("connected to %s", assistNowServer.c_str()));
		
		// download->buffer is subdivided into space for the url buffer, and the rest for the complete header.
		size_t urlBufLen = 256;
		char *urlBuf = (char *)download->buffer;

		size_t requestBufLen = download->bufferSize - urlBufLen;
		char *requestBuf = (char *)&download->buffer[urlBufLen];
		
		UBLOX_DEBUG_VERBOSE(("assistNowKey=%s", assistNowKey.c_str()));

		if (!disableLocation) {
			snprintf(urlBuf, urlBufLen, 
				"/GetOnlineData.ashx?token=%s;gnss=gps;datatype=eph,alm,aux,pos;lat=%.7f;lon=%.7f;pacc=%d;alt=%d;filteronpos;latency=2",
				assistNowKey.c_str(),
				download->lat, 
				download->lng,
				(int) download->accuracy * 2,	// If the accuracy is too small, then the GPS may fail to fix
				(int) download->elev
				);
		}
		else {
			snprintf(urlBuf, urlBufLen, 
				"/GetOnlineData.ashx?token=%s;gnss=gps;datatype=eph,alm,aux",
				assistNowKey.c_str());
		}
		
		// Prepare request
		size_t requestLen = snprintf(requestBuf, requestBufLen, 
			"GET %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"\r\n",
			urlBuf,
			assistNowServer.c_str());

#ifdef UBLOX_DEBUG_VERBOSE_ENABLE
		_log.print(requestBuf);
#endif

		download->client.write((const uint8_t *)requestBuf, requestLen);

		download->inHeader = true;
		download->bufferOffset = 0;

		stateHandler = &UbloxAssistNow::stateReadResponse;
		stateTime = millis();
	}
	else {
		UBLOX_DEBUG(("connection to %s failed", assistNowServer.c_str()) );
		stateHandler = &UbloxAssistNow::stateDone;
	}
}

void UbloxAssistNow::stateReadResponse() {
	if (!download->client.connected()) {
		// Server has disconnected
		UBLOX_DEBUG(("server disconnected unexpectedly"));
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}

	int count = download->client.available();
	if (count <= 0) {
		// TOOD: Handle timeout
		return;
	}


	if (download->inHeader) {
		if (count > (int) (download->bufferSize - download->bufferOffset - 1)) {
			count = (int) (download->bufferSize - download->bufferOffset - 1) ;
		}

		// Log.info("read bufferOffset=%u count=%d", download->bufferOffset, count);

		count = download->client.read(&download->buffer[download->bufferOffset], count);
		if (count <= 0) {
			return;
		}

		// Log.info("actually read %d", count);

		download->bufferOffset += count;
		download->buffer[download->bufferOffset] = 0;

		UBLOX_DEBUG_VERBOSE(("read %d header bytes, received %u so far", count, download->bufferOffset));

		char *endOfHeader = strstr((char *)download->buffer, "\r\n\r\n");
		if (endOfHeader) {
			// We have read the whole header
			endOfHeader += 2;
			*endOfHeader = 0;
			endOfHeader += 2;

#ifdef UBLOX_DEBUG_VERBOSE_ENABLE
			_log.print((const char *) download->buffer);
#endif
			// Parse out things we care about here
			const char *cp = strstr((const char *)download->buffer, "Content-Length:");
			if (cp) {
				cp += 15;
				download->contentLength = (size_t) atoi(cp);
			}

			if (download->contentLength == 0) {
				UBLOX_DEBUG(("Could not parse Content-Length, exiting"));
				stateHandler = &UbloxAssistNow::stateDone;
				return;
			}
			if (download->contentLength > download->bufferSize) {
				UBLOX_DEBUG(("Content-Length of %u is larger than buffer length %u", download->contentLength, download->bufferSize));
				stateHandler = &UbloxAssistNow::stateDone;
				return;
			}

			UBLOX_DEBUG_VERBOSE(("Content-Length is %u", download->contentLength));

			// Move the data after it to the beginning of the buffer
			size_t after = &download->buffer[download->bufferOffset] - (uint8_t *)endOfHeader;
			if (after > 0) {
				memmove(download->buffer, endOfHeader, after);
			}
			download->bufferOffset = after;
			UBLOX_DEBUG_VERBOSE(("Moved %u bytes from after the header to beginning of buffer", after));
			download->inHeader = false;
		}
	}
	else {
		if (count > (int) (download->bufferSize - download->bufferOffset)) {
			count = (int) (download->bufferSize - download->bufferOffset);
		}
		count = download->client.read(&download->buffer[download->bufferOffset], count);
		if (count <= 0) {
			return;
		}

		download->bufferOffset += count;
		UBLOX_DEBUG_VERBOSE(("read %d body bytes, received %u so far", count, download->bufferOffset));

		if (download->bufferOffset >= download->contentLength) {
			UBLOX_DEBUG_VERBOSE(("received bufferOffset=%u contentLength=%u bytes of GPS data", download->bufferOffset, download->contentLength));

			download->client.stop();
			download->bufferOffset = 0;
			stateHandler = &UbloxAssistNow::stateSendToGPS;
			stateTime = 0;
		}

		// 
	}

}

void UbloxAssistNow::stateSendToGPS() {
	if (download->bufferOffset >= download->contentLength) {
		UBLOX_DEBUG(("Done sending aiding data to GPS!"));
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}

	// 
	if (millis() - stateTime < packetDelay) {
		// Have not reached the intra-packet delay yet
		return;
	}
	stateTime = millis();

	if (download->buffer[download->bufferOffset] != 0xb5 ||
		download->buffer[download->bufferOffset + 1] != 0x62) {
#ifdef UBLOX_DEBUG_ENABLE
		_log.info("data didn't begin with sync bytes offset=%u", download->buffer[download->bufferOffset]);
		_log.dump(&download->buffer[download->bufferOffset], 32);
		_log.print("\r\n");
#endif
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}
	uint16_t payloadLen;
	memmove(&payloadLen, &download->buffer[download->bufferOffset + 4], 2);
	
	uint16_t msgLen = payloadLen + 8;
	if ((download->bufferOffset + msgLen) > download->contentLength) {
		UBLOX_DEBUG(("payloadLen of %u seems to be corrupted", payloadLen));
		stateHandler = &UbloxAssistNow::stateDone;
		return;
	}

#ifdef ASSISTNOW_DEBUG_ENABLE

	uint8_t msgClass = download->buffer[download->bufferOffset + 2];
	uint8_t msgId = download->buffer[download->bufferOffset + 3];

	uint8_t msgType = download->buffer[download->bufferOffset + 6];
	uint8_t msgVersion = download->buffer[download->bufferOffset + 7];

	if (msgClass == 0x13 && msgId == 0x40) {
		const char *assistanceType = "unknown";


		switch(msgType) {
			case 0x01:
				assistanceType = "Initial Position";
				break;

			case 0x10:
				assistanceType = "Initial Time";
				break;

			case 0x11:
				assistanceType = "Initial Time GNSS";
				break;

			case 0x30:
				assistanceType = "Earth Orientation";
				break;
		}

		_log.info("Downloading %s assistance msgClass=%02x msgId=%02x msgType=%02x msgVersion=%02x payloadLen=%u", assistanceType, msgClass, msgId, msgType, msgVersion, payloadLen);

	}
	else
	if (msgClass == 0x13) {
		const char *service = 0;
		switch(msgId) {
			case 0x00:
				service = "GPS";
				break;

			case 0x02:
				service = "Galileo";
				break;

			case 0x03:
				service = "BeiDou";
				break;

			case 0x05:
				service = "QZSS";
				break;

			default:
				service = "unnkown service";
				break;
		}

		const char *assistanceType = "unknown";

		if (msgType == 0x01 && msgVersion == 0x00) {
			assistanceType = "ephemeris";
		}
		else
		if (msgType == 0x02 && msgVersion == 0x00) {
			assistanceType = "almanac";
		}
		else
		if (msgType == 0x03 && msgVersion == 0x00) {
			assistanceType = "aux time";
		}
		else
		if (msgType == 0x04 && msgVersion == 0x00) {
			assistanceType = "health";
		}
		else
		if (msgType == 0x05 && msgVersion == 0x00) {
			assistanceType = "UTC";
		}
		else
		if (msgType == 0x06 && msgVersion == 0x00) {
			assistanceType = "ionosphere";
		}
		
		_log.info("Downloading %s %s assistance msgClass=%02x msgId=%02x msgType=%02x msgVersion=%02x payloadLen=%u", service, assistanceType, msgClass, msgId, msgType, msgVersion, payloadLen);
		
	}
	else {
		_log.info("downloading msgClass=%02x msgId=%02x payloadLen=%u", msgClass, msgId, payloadLen);
	}
#endif /* ASSISTNOW_DEBUG_ENABLE */

	AssetTrackerBase::getInstance()->sendCommand(&download->buffer[download->bufferOffset], msgLen);
	download->bufferOffset += msgLen;
}

void UbloxAssistNow::stateDone() {
	if (download) {
		delete download;
		download = 0;
	}
	// Do nothing else
}

void UbloxAssistNow::subscriptionHandler(const char *event, const char *data) {
	// event: hook-response/deviceLocator/<deviceid>/0

	// We use our own subscription handler instead of using the one inside google-maps-device-locator
	// so we can use a single subscription handler for both geolocation and elevation replies. 

	UBLOX_DEBUG_VERBOSE(("subscriptionHandler event=%s data=%s", event, data));

	if (strchr(data, ',')) {
		// float lat, float lon, float accuracy
		char *mutableCopy = strdup(data);
		char *part, *end;
		
		part = strtok_r(mutableCopy, ",", &end);
		if (part) {
			download->lat = atof(part);
			part = strtok_r(NULL, ",", &end);
			if (part) {
				download->lng = atof(part);
				part = strtok_r(NULL, ",", &end);
				if (part) {
					download->accuracy = atof(part);

					// (*callback)(lat, lon, accuracy);
					snprintf((char *)download->buffer, download->bufferSize, "{\"lat\":%f, \"lng\":%f}", download->lat, download->lng);
		
					UBLOX_DEBUG(("requesting elevation for %s", (const char *)download->buffer));	

					Particle.publish("AssistElevation", (char *)download->buffer, PRIVATE);
				}
			}
		}

		free(mutableCopy);
	}
	else {
		// Elevation response only contains the elevation
		download->elev = atof(data);
		UBLOX_DEBUG(("elevation=%f", download->elev));

		stateHandler = &UbloxAssistNow::stateSendRequest;
	}
}


AssistNowDownload::AssistNowDownload() {
}

AssistNowDownload::~AssistNowDownload() {
	if (buffer) {
		delete[] buffer;
	}
}

bool AssistNowDownload::alloc(size_t bufferSize) {
	this->bufferSize = bufferSize;
	this->buffer = new uint8_t[bufferSize];

	return (this->buffer != NULL);
}



