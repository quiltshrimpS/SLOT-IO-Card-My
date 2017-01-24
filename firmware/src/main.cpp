/**
 * D2 - 74HC595 - SDI
 * D3 - 74HC595 - SRCLK
 * D4 - 74HC595 - RCLK
 * D5 - 74HC595 - nCLR
 * D6 - 74HC595 - nG
 * D7 - 74HC165 - QH
 * D8 - 74HC165 - CLK
 * D9 - 74HC165 - SH/nLD
 *
 * A4 - FRAM - SDA
 * A5 - FRAM - SCL
 * A7 - FRAM - WP
 */

#include <Arduino.h>
#include <DigitalIO.h>
#include <Wire.h>
#include <FRAM_MB85RC_I2C.h>

#include <CmdMessenger.h>

#include "Communication.h"
#include "WreckedSPI.h"
#include "Ports.h"
#include "Debounce.h"
#include "Pulse.h"
#include "Configuration.h"

FRAM_MB85RC_I2C fram(MB85RC_DEFAULT_ADDRESS, true, /* WP */ A7, 16 /* kb */);
WreckedSPI< /* MISO */ 7, /* MOSI */ 2, /* SCLK_MISO */ 8, /* SCLK_MOSI */ 3, /* MODE_MISO */ 2, /* MODE_MOSI */ 0 > spi;
Configuration conf(fram);

CmdMessenger messenger(Serial);
Communicator communicator(messenger);

union {
    uint8_t bytes[3];
    struct OutPort port;
} out;

union {
    uint8_t bytes[3];
    struct InPort port;
} in, previous_in;

bool do_send = false;

Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_score;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_wash;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_insert;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_eject;

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_banknote(
	nullptr,
	[] () {
		uint32_t coins = conf.getCoinCount(TRACK_BANKNOTE) + 1;
		conf.setCoinCount(TRACK_BANKNOTE, coins);
		communicator.dispatchCoinCounterResult(TRACK_BANKNOTE, coins);
	}
);

Debounce<HIGH, DEBOUNCE_TIMEOUT> debounce_eject(
	nullptr,
	[] () {
		uint32_t coins = conf.getCoinCount(TRACK_EJECT) + 1;
		conf.setCoinCount(TRACK_EJECT, coins);
		pulse_counter_eject.pulse(1);
		uint8_t to_eject = conf.getCoinsToEject(TRACK_EJECT);
		if (to_eject < 2) {
			out.port.ssr1 = false; // pull LOW to stop the motor
			do_send = true;
		}
		if (to_eject > 0)
			conf.setCoinsToEject(TRACK_EJECT, to_eject - 1);
		communicator.dispatchCoinCounterResult(TRACK_EJECT, coins);
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_1(
	nullptr,
	[] () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_1) + 1;
		conf.setCoinCount(TRACK_INSERT_1, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_1, coins);
		pulse_counter_insert.pulse(1);
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_2(
	nullptr,
	[] () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_2) + 1;
		conf.setCoinCount(TRACK_INSERT_2, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_2, coins);
		pulse_counter_insert.pulse(1);
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_3(
	nullptr,
	[] () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_3) + 1;
		conf.setCoinCount(TRACK_INSERT_3, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_3, coins);
		pulse_counter_insert.pulse(1);
	}
);

static uint8_t const PIN_LATCH_OUT = 4; // for 74HC595
static uint8_t const PIN_LATCH_IN = 9;  // for 74HC165

void setup() {
	// FIXME: write the SPI as early as possible, because 74HC595 defaults to
	// FIXME: HIGH on power-on. pull it low as early as possible.
    fastPinConfig(5, OUTPUT, LOW); // 595 nCLR (active LOW), clear register
    fastPinConfig(6, OUTPUT, LOW); // 595 nG (active LOW), enable
    fastPinConfig(PIN_LATCH_OUT, OUTPUT, HIGH); // 595 latch
    fastPinConfig(PIN_LATCH_IN, OUTPUT, LOW);   // 165 latch
    spi.begin(); // for 74HC595 and 74HC165
    fastDigitalWrite(5, HIGH); // cleared 74HC595, put it back on.
	// read the initial states, and write the initial states.
    fastDigitalWrite(PIN_LATCH_OUT, LOW);
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    previous_in.bytes[0] = spi.transfer(out.bytes[0]) | IN_MASK_0;
    previous_in.bytes[1] = spi.transfer(out.bytes[1]) | IN_MASK_1;
    previous_in.bytes[2] = spi.transfer(out.bytes[2]) | IN_MASK_2;
    fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros(); // record the time early, for better accuracy

	// do the rest of the thing after we switch off the motor
    Serial.begin(UART_BAUDRATE);

    Wire.begin(); // for FRAM
	Wire.setClock(TWI_BAUDRATE);
    fram.begin();
	conf.begin();

	// populate the debouncers
	debounce_banknote.begin(in.port.sw20, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_insert_1.begin(in.port.sw12, now);
	debounce_insert_2.begin(in.port.sw13, now);
	debounce_insert_3.begin(in.port.sw14, now);

	messenger.attach([]() {
		switch (messenger.commandID()) {
			case CMD_GET_INFO:
				communicator.dispatchGetInfoResult();
				break;
			case CMD_EJECT_COIN:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					uint8_t const count = messenger.readBinArg<uint8_t>();

					// block newer command if there are still coins left to be ejected
					uint8_t const coins = conf.getCoinsToEject(TRACK_EJECT);
					if (count != 0 && coins != 0) {
						communicator.dispatchErrorEjectInterrupted(track, coins);
					} else if (!conf.setCoinsToEject(track, count)) {
						communicator.dispatchErrorNotATrack(track);
					} else if (count != 0) {
						out.port.ssr1 = true;
						do_send = true;
					}
				}
				break;
			case CMD_GET_COIN_COUNTER:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					communicator.dispatchCoinCounterResult(track, conf.getCoinCount(track));
				}
				break;
			case CMD_RESET_COIN_COINTER:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (!conf.setCoinCount(track, 0))
						communicator.dispatchErrorNotATrack(track);
				}
				break;
			case CMD_GET_KEYS:
				communicator.dispatchKey(3, previous_in.bytes);
				break;
			case CMD_WRITE_STORAGE:
				{
					uint16_t const address = messenger.readBinArg<uint16_t>();
					uint8_t const length = messenger.readBinArg<uint8_t>();
					if (address < 0x200) {
						communicator.dispatchErrorProtectedStorage(address);
					} else if (length > MAX_BYTES_LENGTH) {
						communicator.dispatchErrorTooLong(length);
					} else {
						uint8_t buffer[length];
						for (uint8_t i = 0; i < length; ++i)
							buffer[i] = messenger.readBinArg<uint8_t>();
						conf.writeBytes(address, length, buffer);
						communicator.dispatchWriteStorageResult(address, length);
					}
				}
				break;
			case CMD_READ_STORAGE:
				{
					uint16_t const address = messenger.readBinArg<uint16_t>();
					uint8_t const length = messenger.readBinArg<uint8_t>();
					if (address < 0x200) {
						communicator.dispatchErrorProtectedStorage(address);
					} else if (length > MAX_BYTES_LENGTH) {
						communicator.dispatchErrorTooLong(length);
					} else {
						uint8_t buffer[length];
						conf.readBytes(address, length, buffer);
						communicator.dispatchReadStorageResult(address, length, buffer);
					}
				}
				break;
			default:
				communicator.dispatchErrorUnknownCommand(messenger.commandID());
		}
	});
}

void loop() {
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);
	uint32_t now = micros();
	Configuration::TrackLevelsT &track_levels = conf.getTrackLevels();
	debounce_insert_1.feed(in.port.sw12, track_levels.bits.track_level_0, now);
	debounce_insert_2.feed(in.port.sw13, track_levels.bits.track_level_1, now);
	debounce_insert_3.feed(in.port.sw14, track_levels.bits.track_level_2, now);
	debounce_banknote.feed(in.port.sw20, track_levels.bits.track_level_3, now);
	debounce_eject.feed(in.port.sw11, track_levels.bits.track_level_4, now);
	if (pulse_counter_score.update(now))
	{
		out.port.counter1 = pulse_counter_score.get();
		do_send = true;
	}
	if (pulse_counter_wash.update(now))
	{
		out.port.counter2 = pulse_counter_wash.get();
		do_send = true;
	}
	if (pulse_counter_insert.update(now))
	{
		out.port.counter3 = pulse_counter_insert.get();
		do_send = true;
	}
	if (pulse_counter_eject.update(now))
	{
		out.port.counter4 = pulse_counter_eject.get();
		do_send = true;
	}

	// rest of the keys are not debounced, we just send them to the PC if
	// anything changed.
	uint8_t masked[] = {
		static_cast<uint8_t>(in.bytes[0] & IN_MASK_0),
		static_cast<uint8_t>(in.bytes[1] & IN_MASK_1),
		static_cast<uint8_t>(in.bytes[2] & IN_MASK_2),
	};
	if (masked[0] != previous_in.bytes[0] ||
		masked[1] != previous_in.bytes[1] ||
		masked[2] != previous_in.bytes[2])
	{
		communicator.dispatchKey(3, masked);

		previous_in.bytes[0] = masked[0];
		previous_in.bytes[1] = masked[1];
		previous_in.bytes[2] = masked[2];
	}

	// feed the serial data before we send, because messenger might want to
	// modify stuff.
	messenger.feedinSerialData();

    if (do_send) {
		do_send = false;
        fastDigitalWrite(PIN_LATCH_OUT, LOW);
        spi.send(out.bytes[0]);
        spi.send(out.bytes[1]);
        spi.send(out.bytes[2]);
        fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    }
}
