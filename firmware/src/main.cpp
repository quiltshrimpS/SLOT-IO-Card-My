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

#include <avr/power.h>

#include "Communication.h"
#include "WreckedSPI.h"
#include "Ports.h"
#include "Debounce.h"
#include "Pulse.h"
#include "Configuration.h"
#include "TimeoutTracker.h"

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

// put these here so we can iterate through it...
static const uint8_t OUTPUT_MASK[3] = { OUT_MASK_0, OUT_MASK_1, OUT_MASK_2 };

bool do_send = false;

TimeoutTracker tracker_nack(
	#if defined(DEBUG_SERIAL)
	"NACK tracker"
	#endif
);
TimeoutTracker tracker_eject(
	#if defined(DEBUG_SERIAL)
	"eject tracker"
	#endif
);

Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_score;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_wash;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_insert;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_eject;

class EmptyFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () { }
};

class DebounceBanknoteFallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		uint32_t coins = conf.getCoinCount(TRACK_BANKNOTE) + 1;
		conf.setCoinCount(TRACK_BANKNOTE, coins);
		communicator.dispatchCoinCounterResult(TRACK_BANKNOTE, coins);
	}
};
Debounce<LOW, DEBOUNCE_TIMEOUT, EmptyFunctorT, DebounceBanknoteFallFunctorT> debounce_banknote;

class DebounceEjectFallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		uint32_t coins = conf.getCoinCount(TRACK_EJECT) + 1;
		conf.setCoinCount(TRACK_EJECT, coins);
		pulse_counter_eject.pulse(1);
		uint8_t to_eject = conf.getCoinsToEject(TRACK_EJECT);
		if (to_eject < 2) {
			tracker_eject.stop();
			tracker_nack.stop();
			out.port.ssr1 = false; // pull LOW to stop the motor
			do_send = true;
		}
		if (to_eject > 0) {
			tracker_eject.start();
			conf.setCoinsToEject(TRACK_EJECT, to_eject - 1);
		}
		communicator.dispatchCoinCounterResult(TRACK_EJECT, coins);
		tracker_nack.start();
	}
};
Debounce<LOW, DEBOUNCE_TIMEOUT, EmptyFunctorT, DebounceEjectFallFunctorT> debounce_eject;

class DebounceInsert1FallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_1) + 1;
		conf.setCoinCount(TRACK_INSERT_1, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_1, coins);
		pulse_counter_insert.pulse(1);
	}
};
Debounce<LOW, DEBOUNCE_TIMEOUT, EmptyFunctorT, DebounceInsert1FallFunctorT> debounce_insert_1;

class DebounceInsert2FallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_2) + 1;
		conf.setCoinCount(TRACK_INSERT_2, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_2, coins);
		pulse_counter_insert.pulse(1);
	}
};
Debounce<LOW, DEBOUNCE_TIMEOUT, EmptyFunctorT, DebounceInsert2FallFunctorT> debounce_insert_2;

class DebounceInsert3FallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		uint32_t coins = conf.getCoinCount(TRACK_INSERT_3) + 1;
		conf.setCoinCount(TRACK_INSERT_3, coins);
		communicator.dispatchCoinCounterResult(TRACK_INSERT_3, coins);
		pulse_counter_insert.pulse(1);
	}
};
Debounce<LOW, DEBOUNCE_TIMEOUT, EmptyFunctorT, DebounceInsert3FallFunctorT> debounce_insert_3;

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

	power_adc_disable(); // we're not using the ADC module
	power_spi_disable(); // we're not using the SPI module
	power_timer1_disable(); // we're not using timer1
	power_timer2_disable(); // we're not using timer2

	// do the rest of the thing after we switch off the motor
    Serial.begin(UART_BAUDRATE);

    Wire.begin(); // for FRAM
	Wire.setClock(TWI_BAUDRATE);
    fram.begin();
	conf.begin();

	// initialize timeout trackers
	tracker_eject.begin(conf.getEjectTimeout(TRACK_EJECT));
	tracker_nack.begin(TIMEOUT_NACK);

	// populate the debouncers
	debounce_banknote.begin(in.port.sw20, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_insert_1.begin(in.port.sw12, now);
	debounce_insert_2.begin(in.port.sw13, now);
	debounce_insert_3.begin(in.port.sw14, now);

	messenger.attach([]() {
		#if defined(DEBUG_SERIAL)
		uint32_t t1, t2;
		t1 = micros();
		#endif
		switch (messenger.commandID()) {
			case CMD_ACK:
				tracker_nack.stop();
				break;
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
						tracker_eject.start();
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
			case CMD_SET_OUTPUT:
				{
					uint8_t const length = messenger.readBinArg<uint8_t>();
					if (length != 0) {
						for (int i = 0;i < length && i < 3;++i)
							out.bytes[i] =
								(out.bytes[i] & ~OUTPUT_MASK[i]) |
								(messenger.readBinArg<uint8_t>() & OUTPUT_MASK[i]);
						do_send = true;
					}
				}
				break;
			case CMD_TICK_COUNTER:
				{
					uint8_t const counter = messenger.readBinArg<uint8_t>();
					uint8_t const ticks = messenger.readBinArg<uint8_t>();
					if (counter == 1) {
						pulse_counter_score.pulse(ticks);
					} else if (counter == 2) {
						pulse_counter_wash.pulse(ticks);
				#if defined(DEBUG_SERIAL)
					} else if (counter == 3) {
						pulse_counter_insert.pulse(ticks);
					} else if (counter == 4) {
						pulse_counter_eject.pulse(ticks);
				#endif
					} else {
						communicator.dispatchErrorNotACounter(counter);
					}
				}
				break;
			case CMD_SET_TRACK_LEVEL:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					uint8_t const level = messenger.readBinArg<bool>();
					if (!conf.setTrackLevel(track, level))
						communicator.dispatchErrorNotATrack(track);
				}
				break;
			case CMD_SET_EJECT_TIMEOUT:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					uint32_t const timeout = messenger.readBinArg<uint32_t>();
					if (!conf.setEjectTimeout(track, timeout))
						communicator.dispatchErrorNotATrack(track);
					else
						tracker_eject.begin(timeout);
				}
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
		#if defined(DEBUG_SERIAL)
		t2 = micros();
		DEBUG_SERIAL.print(F("90,cmd handler took "));
		DEBUG_SERIAL.print(t2 - t1);
		DEBUG_SERIAL.print("us;");
		#endif
	});
}

void loop() {
	#if defined(DEBUG_SERIAL)
	static uint32_t last_millis = millis();
	uint32_t t1, t2;
	t1 = micros();
	#endif
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros();

	// check the timeout tracker before we feed the debouncers, since debouncers
	// might trigger tracker.start() when a coin is confirmed.
	//
	// run the NACK tracker earlier, since eject tracker might write the FRAM
	// which takes a lot of time.
	if (tracker_nack.trigger(now))
	{
		tracker_eject.stop();
		out.port.ssr1 = false;
		do_send = true;
	}
	if (tracker_eject.trigger(now))
	{
		uint8_t const coins = conf.getCoinsToEject(TRACK_EJECT);
		if (coins) {
			communicator.dispatchErrorEjectTimeout(TRACK_EJECT, coins);
			out.port.ssr1 = false;
			do_send = true;
		}
	}

	Configuration::TrackLevelsT &track_levels = conf.getTrackLevels();
	debounce_insert_1.feed(in.port.sw12, track_levels.bits.track_level_0, now);
	debounce_insert_2.feed(in.port.sw13, track_levels.bits.track_level_1, now);
	debounce_insert_3.feed(in.port.sw14, track_levels.bits.track_level_2, now);
	debounce_banknote.feed(in.port.sw20, track_levels.bits.track_level_3, now);
	debounce_eject.feed(in.port.sw11, track_levels.bits.track_level_4, now);

	if (pulse_counter_score.update(now))
	{
		out.port.counter1 = pulse_counter_score.get();
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("90,pulsing counter1 = "));
		DEBUG_SERIAL.print(out.port.counter1 ? F("HIGH;") : F("LOW;"));
		#endif
		do_send = true;
	}
	if (pulse_counter_wash.update(now))
	{
		out.port.counter2 = pulse_counter_wash.get();
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("90,pulsing counter2 = "));
		DEBUG_SERIAL.print(out.port.counter2 ? F("HIGH;") : F("LOW;"));
		#endif
		do_send = true;
	}
	if (pulse_counter_insert.update(now))
	{
		out.port.counter3 = pulse_counter_insert.get();
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("90,pulsing counter3 = "));
		DEBUG_SERIAL.print(out.port.counter3 ? F("HIGH;") : F("LOW;"));
		#endif
		do_send = true;
	}
	if (pulse_counter_eject.update(now))
	{
		out.port.counter4 = pulse_counter_eject.get();
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("90,pulsing counter4 = "));
		DEBUG_SERIAL.print(out.port.counter4 ? F("HIGH;") : F("LOW;"));
		#endif
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

	#if defined(DEBUG_SERIAL)
	t2 = micros();
	if (millis() - last_millis > 1000) {
		last_millis = millis();
		DEBUG_SERIAL.print(F("90,loop() took "));
		DEBUG_SERIAL.print(t2 - t1);
		DEBUG_SERIAL.print("us;");
	}
	#endif
}
