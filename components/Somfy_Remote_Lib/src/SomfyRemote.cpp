#include "SomfyRemote.h"

#include <vector>

#include "esp_log.h"

#define SYMBOL 640

static const char *TAG = "SomfyRemote";

SomfyRemote::SomfyRemote(byte emitterPin, uint32_t remote, RollingCodeStorage *rollingCodeStorage)
	: emitterPin(emitterPin), remote(remote), rollingCodeStorage(rollingCodeStorage) {}

void SomfyRemote::setup() {
	// RMT initialization is done once, in RemoteDeviceManager::begin(), after the
	// CC1101 is configured (CC1101::setGDO() calls pinMode() on the GDO0 pin,
	// which would otherwise steal the pin back from the RMT peripheral).
}

void SomfyRemote::sendCommand(Command command, int repeat) {
	const uint16_t rollingCode = rollingCodeStorage->nextCode();
	sendCommandWithCode(command, rollingCode, repeat);
}

void SomfyRemote::sendCommandWithCode(Command command, uint16_t rollingCode, int repeat) {
	byte frame[7];
	buildFrame(frame, command, rollingCode);

	// Wake-up pulse & silence (only before the first frame).
	rmt_data_t wakeup = {.duration0 = 9415, .level0 = 1, .duration1 = 9565, .level1 = 0};
	rmtWrite(emitterPin, &wakeup, 1, RMT_WAIT_FOR_EVER);
	delay(80);

	// Build the entire multi-frame transmission as one continuous RMT sequence.
	//
	// We build a flat list of phases (each a duration + level half-symbol), then
	// pack them pairwise into RMT symbols. This avoids placing a zero-duration
	// field mid-sequence (which the RMT peripheral treats as end-of-transmission).
	//
	// The Somfy protocol requires 2 hardware sync pulses before the first frame
	// and 7 before subsequent frames. We achieve this by structuring each repeated
	// chunk as: [2 sync + sw sync + data + inter-frame silence + 5 sync]
	//
	// When repeated, the trailing 5 sync from one chunk plus the leading 2 sync
	// from the next chunk gives the required 7 sync for subsequent frames.

	struct Phase {
		uint16_t duration;
		uint8_t level;
	};

	std::vector<Phase> phases;

	for (int n = 0; n < 1 + repeat; n++) {
		// Hardware sync: 2 pulses
		for (int i = 0; i < 2; i++) {
			phases.push_back({4 * SYMBOL, 1});
			phases.push_back({4 * SYMBOL, 0});
		}

		// Software sync
		phases.push_back({4550, 1});
		phases.push_back({SYMBOL, 0});

		// Data: bits are sent one by one, starting with the MSB.
		for (byte i = 0; i < 56; i++) {
			if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
				phases.push_back({SYMBOL, 0});
				phases.push_back({SYMBOL, 1});
			} else {
				phases.push_back({SYMBOL, 1});
				phases.push_back({SYMBOL, 0});
			}
		}

		// Inter-frame silence + 5 trailing hardware sync pulses (which combine
		// with the next chunk's 2 leading sync to form 7 sync pulses).
		phases.push_back({30415, 0});
		for (int i = 0; i < 5; i++) {
			phases.push_back({4 * SYMBOL, 1});
			phases.push_back({4 * SYMBOL, 0});
		}
	}

	// Pack phases pairwise into RMT symbols.
	std::vector<rmt_data_t> symbols;
	for (size_t i = 0; i < phases.size(); i += 2) {
		rmt_data_t sym = {};
		sym.duration0 = phases[i].duration;
		sym.level0 = phases[i].level;
		if (i + 1 < phases.size()) {
			sym.duration1 = phases[i + 1].duration;
			sym.level1 = phases[i + 1].level;
		}
		symbols.push_back(sym);
	}

	ESP_LOGI(TAG, "transmitting %u RMT symbols (%d frames) on pin %d",
			 (unsigned)symbols.size(), 1 + repeat, emitterPin);
	bool ok = rmtWrite(emitterPin, symbols.data(), symbols.size(), RMT_WAIT_FOR_EVER);
	ESP_LOGI(TAG, "rmtWrite returned %s", ok ? "true" : "false");

	// Inter-command silence so that a subsequent command's wake-up pulse is not
	// mistaken for part of this transmission.
	delay(80);
}

void SomfyRemote::printFrame(byte *frame) {
	for (byte i = 0; i < 7; i++) {
		if (frame[i] >> 4 == 0) {  //  Displays leading zero in case the most significant
			Serial.print("0");     // nibble is a 0.
		}
		Serial.print(frame[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
}

void SomfyRemote::buildFrame(byte *frame, Command command, uint16_t code) {
	const byte button = static_cast<byte>(command);
	frame[0] = 0xA7;          // Encryption key. Doesn't matter much
	frame[1] = button << 4;   // Which button did  you press? The 4 LSB will be the checksum
	frame[2] = code >> 8;     // Rolling code (big endian)
	frame[3] = code;          // Rolling code
	frame[4] = remote >> 16;  // Remote address
	frame[5] = remote >> 8;   // Remote address
	frame[6] = remote;        // Remote address

#ifdef DEBUG
	Serial.print("Frame         : ");
	printFrame(frame);
#endif

	// Checksum calculation: a XOR of all the nibbles
	byte checksum = 0;
	for (byte i = 0; i < 7; i++) {
		checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
	}
	checksum &= 0b1111;  // We keep the last 4 bits only

	// Checksum integration
	frame[1] |= checksum;

#ifdef DEBUG
	Serial.print("With checksum : ");
	printFrame(frame);
#endif

	// Obfuscation: a XOR of all the bytes
	for (byte i = 1; i < 7; i++) {
		frame[i] ^= frame[i - 1];
	}

#ifdef DEBUG
	Serial.print("Obfuscated    : ");
	printFrame(frame);
#endif
}

void SomfyRemote::sendFrame(byte *frame, byte sync) {
	// No longer used — transmission is handled entirely by sendCommandWithCode.
}

Command getSomfyCommand(const String &string) {
	if (string.equalsIgnoreCase("My")) {
		return Command::My;
	} else if (string.equalsIgnoreCase("Up")) {
		return Command::Up;
	} else if (string.equalsIgnoreCase("MyUp")) {
		return Command::MyUp;
	} else if (string.equalsIgnoreCase("Down")) {
		return Command::Down;
	} else if (string.equalsIgnoreCase("MyDown")) {
		return Command::MyDown;
	} else if (string.equalsIgnoreCase("UpDown")) {
		return Command::UpDown;
	} else if (string.equalsIgnoreCase("Prog")) {
		return Command::Prog;
	} else if (string.equalsIgnoreCase("SunFlag")) {
		return Command::SunFlag;
	} else if (string.equalsIgnoreCase("Flag")) {
		return Command::Flag;
	} else if (string.length() == 1) {
		return static_cast<Command>(strtol(string.c_str(), nullptr, 16));
	} else {
		return Command::My;
	}
}
