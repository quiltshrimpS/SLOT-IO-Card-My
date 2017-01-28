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
#include <avr/wdt.h>

#include "util.h"

#include "Communication.h"

#include "WreckedSPI.h"
#include "Ports.h"
#include "Debounce.h"
#include "Pulse.h"
#include "Configuration.h"
#include "TimeoutTracker.h"
#include "Communicator.h"

typedef WreckedSPI< /* MISO */ 7, /* MOSI */ 2, /* SCLK_MISO */ 8, /* SCLK_MOSI */ 3, /* MODE_MISO */ 2, /* MODE_MOSI */ 0 > spi;
Configuration conf;

CmdMessenger messenger(Serial);
Communicator communicator(messenger);

union {
    uint8_t bytes[sizeof(struct OutPort)];
    struct OutPort port;
} out;

union {
    uint8_t bytes[sizeof(struct InPort)];
    struct InPort port;
} in, previous_in;

// put these here so we can iterate through it...
static const uint8_t OUTPUT_MASK[3] = { OUT_MASK_0, OUT_MASK_1, OUT_MASK_2 };

bool do_send = false;

TimeoutTracker trackers[] = {
#if defined(DEBUG_SERIAL)
	TimeoutTracker("eject tracker"),
	TimeoutTracker("ticket tracker"),
	TimeoutTracker("NACK tracker"),
#else
	TimeoutTracker(),
	TimeoutTracker(),
	TimeoutTracker(),
#endif
};
#define TRACKER_EJECT (trackers[0])
#define TRACKER_TICKET (trackers[1])
#define TRACKER_NACK (trackers[2])

Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counters[4];

class EmptyFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () { }
};

template < uint8_t TRACK, uint8_t COUNTER >
class DebounceEjectFallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		if (TRACK != TRACK_NOT_A_TRACK) {
			uint32_t coins = conf.getCoinCount(TRACK) + 1;
			conf.setCoinCount(TRACK, coins);
			uint8_t to_eject = conf.getCoinsToEject(TRACK);
			if (to_eject < 2) {
				trackers[TRACK].stop();
				TRACKER_NACK.stop();
				#if (NUM_EJECT_TRACKS < 4)
				bitClear(out.bytes[0], 7 - TRACK); // pull LOW to stop the SSR
				#else
				#error find another way to clear the bits!
				#endif
				do_send = true;
			}
			if (to_eject > 0) {
				trackers[TRACK].stop();
				conf.setCoinsToEject(TRACK, to_eject - 1);
			}
			communicator.dispatchCoinCounterResult(TRACK, coins);
			TRACKER_NACK.start();
		}
		if (COUNTER != COUNTER_NOT_A_COUNTER) {
			pulse_counters[COUNTER].pulse(1);
		}
	}
};

Debounce<
	LOW, DEBOUNCE_TIMEOUT,
	EmptyFunctorT,
	DebounceEjectFallFunctorT<TRACK_EJECT, COUNTER_EJECT>
> debounce_eject;

Debounce<
	LOW, DEBOUNCE_TIMEOUT,
	EmptyFunctorT,
	DebounceEjectFallFunctorT<TRACK_TICKET, COUNTER_NOT_A_COUNTER>
> debounce_ticket;

template < uint8_t TRACK, uint8_t COUNTER >
class DebounceInsertFallFunctorT {
public:
	__attribute__((always_inline)) inline
	void operator () () {
		if (TRACK != TRACK_NOT_A_TRACK) {
			uint32_t coins = conf.getCoinCount(TRACK) + 1;
			conf.setCoinCount(TRACK, coins);
			communicator.dispatchCoinCounterResult(TRACK, coins);
		}
		if (COUNTER != COUNTER_NOT_A_COUNTER) {
			pulse_counters[COUNTER].pulse(1);
		}
	}
};

Debounce<
	LOW, DEBOUNCE_TIMEOUT,
	EmptyFunctorT,
	DebounceInsertFallFunctorT<TRACK_INSERT_1, COUNTER_INSERT>
> debounce_insert_1;

Debounce<
	LOW, DEBOUNCE_TIMEOUT,
	EmptyFunctorT,
	DebounceInsertFallFunctorT<TRACK_INSERT_2, COUNTER_INSERT>
> debounce_insert_2;

Debounce<
	LOW, DEBOUNCE_TIMEOUT,
	EmptyFunctorT,
	DebounceInsertFallFunctorT<TRACK_BANKNOTE, COUNTER_NOT_A_COUNTER>
> debounce_banknote;

static uint8_t const PIN_LATCH_OUT = 4; // for 74HC595
static uint8_t const PIN_LATCH_IN = 9;  // for 74HC165

void setup() {
	#if defined(DEBUG_SERIAL)
	uint32_t t1 = micros(), t2;
	#endif
	// FIXME: because 74HC595 defaults to HIGH on power-on, send the initial
	// FIXME: states as soon as we enter `setup()`
    fastPinConfig(5, OUTPUT, LOW); // 595 nCLR (active LOW), clear register
    fastPinConfig(6, OUTPUT, LOW); // 595 nG (active LOW), enable
    fastPinConfig(PIN_LATCH_OUT, OUTPUT, HIGH); // 595 latch
    fastPinConfig(PIN_LATCH_IN, OUTPUT, LOW);   // 165 latch
    spi::begin(); // for 74HC595 and 74HC165
    fastDigitalWrite(5, HIGH); // cleared 74HC595, put it back on.

	// send and receive the initial states
    fastDigitalWrite(PIN_LATCH_OUT, LOW);
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    previous_in.bytes[0] = spi::transfer(out.bytes[0]) & IN_MASK_0;
    previous_in.bytes[1] = spi::transfer(out.bytes[1]) & IN_MASK_1;
    previous_in.bytes[2] = spi::transfer(out.bytes[2]) & IN_MASK_2;
    fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros(); // record the time early, for better accuracy

	// power-off unused peripherals, so they don't generate interrupts
	// also saves some power...
	power_adc_disable(); // we're not using the ADC
	power_spi_disable(); // we're not using the hardware SPI
	power_timer1_disable(); // we're not using Timer1
	power_timer2_disable(); // we're not using Timer2

	// debuggin with FRAM takes a lot of time, enable wdt after that.
	#if !defined(DEBUG_SERIAL)
	wdt_enable(WDTO_15MS);
	#endif

	// do the rest of the thing after we switch off the motor
    Serial.begin(UART_BAUDRATE);
	conf.begin();

	// debuggin with FRAM takes a lot of time, enable wdt after that.
	#if defined(DEBUG_SERIAL)
	wdt_enable(WDTO_60MS);
	#endif

	// initialize timeout trackers
	TRACKER_EJECT.begin(conf.getEjectTimeout(TRACK_EJECT));
	TRACKER_TICKET.begin(conf.getEjectTimeout(TRACK_TICKET));
	TRACKER_NACK.begin(TIMEOUT_NACK);

	// populate the debouncers
	debounce_banknote.begin(in.port.sw20, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_ticket.begin(in.port.sw14, now);
	debounce_insert_1.begin(in.port.sw12, now);
	debounce_insert_2.begin(in.port.sw13, now);

	// attach command handler
	messenger.attach([]() {
		#if defined(DEBUG_SERIAL)
		uint32_t t1, t2;
		t1 = micros();
		#endif
		switch (messenger.commandID()) {
			case CMD_ACK:
				TRACKER_NACK.stop();
				break;
			case CMD_GET_INFO:
				communicator.dispatchGetInfoResult();
				break;
			case CMD_EJECT_COIN:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (unlikely(track >= NUM_EJECT_TRACKS)) {
						communicator.dispatchErrorNotATrack(track);
					} else {
						uint8_t const count = messenger.readBinArg<uint8_t>();
						uint8_t const remained = conf.getCoinsToEject(track);

						// block newer command if there are still something left to be ejected
						if (count != 0 && remained != 0) {
							communicator.dispatchErrorEjectInterrupted(track, remained);
						} else {
							conf.setCoinsToEject(track, count);
							if (likely(count != 0)) {
								trackers[track].start();
								#if (NUM_EJECT_TRACKS < 4)
								bitSet(out.bytes[0], 7 - track); // pull HIGH to enable the SSR
								#else
								#error find another way to set the bits!
								#endif
								do_send = true;
							} else {
								trackers[track].stop();
								TRACKER_NACK.stop();
								#if (NUM_EJECT_TRACKS < 4)
								bitClear(out.bytes[0], 7 - track); // pull LOW to stop the SSR
								#else
								#error find another way to clear the bits!
								#endif
								do_send = true;
							}
						}
					}
				}
				break;
			case CMD_GET_COIN_COUNTER:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (unlikely(track >= NUM_TRACKS)) {
						communicator.dispatchErrorNotATrack(track);
					} else {
						communicator.dispatchCoinCounterResult(track, conf.getCoinCount(track));
					}
				}
				break;
			case CMD_RESET_COIN_COINTER:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (unlikely(track >= NUM_TRACKS)) {
						communicator.dispatchErrorNotATrack(track);
					} else {
						conf.setCoinCount(track, 0);
					}
				}
				break;
			case CMD_GET_KEY_MASKS:
				communicator.dispatchKeyMasksResult();
				break;
			case CMD_GET_KEYS:
				communicator.dispatchKeysResult(3, previous_in.bytes);
				break;
			case CMD_SET_OUTPUT:
				{
					uint8_t const length = messenger.readBinArg<uint8_t>();
					if (unlikely(length != 0)) {
						for (int i = 0;i < length && i < 3;++i)
							out.bytes[i] =
								(out.bytes[i] & ~OUTPUT_MASK[i]) |
								(messenger.readBinArg<uint8_t>() & OUTPUT_MASK[i]);
						do_send = true;
					}
				}
				break;
			case CMD_TICK_AUDIT_COUNTER:
				{
					uint8_t const counter = messenger.readBinArg<uint8_t>();
					uint32_t const ticks = messenger.readBinArg<uint32_t>();
				#if defined(DEBUG_SERIAL)
					if (likely(counter < 4)) {
				#else
					if (likely(counter < 2)) {
				#endif
						pulse_counters[counter].pulse(ticks);
					} else {
						communicator.dispatchErrorNotACounter(counter);
					}
				}
				break;
			case CMD_SET_TRACK_LEVEL:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (unlikely(track >= NUM_TRACKS)) {
						communicator.dispatchErrorNotATrack(track);
					} else {
						uint8_t const level = messenger.readBinArg<bool>();
						conf.setTrackLevel(track, level);
					}
				}
				break;
			case CMD_SET_EJECT_TIMEOUT:
				{
					uint8_t const track = messenger.readBinArg<uint8_t>();
					if (unlikely(track >= NUM_EJECT_TRACKS)) {
						communicator.dispatchErrorNotATrack(track);
					} else {
						uint32_t const timeout = messenger.readBinArg<uint32_t>();
						conf.setEjectTimeout(track, timeout);
					}
				}
				break;
			case CMD_WRITE_STORAGE:
				{
					uint32_t const address = messenger.readBinArg<uint16_t>();
					uint8_t const length = messenger.readBinArg<uint8_t>();
					if (unlikely(address < CONF_ADDR_USER_BEGIN)) {
						communicator.dispatchErrorProtectedStorage(address);
					} else if (unlikely(length > MAX_BYTES_LENGTH)) {
						communicator.dispatchErrorTooLong(length);
					} else if (unlikely(address + length > MAX_STORAGE_ADDRESS)) {
						communicator.dispatchErrorOutOfRange(address, length);
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
					uint32_t const address = messenger.readBinArg<uint16_t>();
					uint8_t const length = messenger.readBinArg<uint8_t>();
				#if !defined(DEBUG_SERIAL)
					if (unlikely(address < CONF_ADDR_USER_BEGIN)) {
						communicator.dispatchErrorProtectedStorage(address);
					} else
				#endif
					if (unlikely(length > MAX_BYTES_LENGTH)) {
						communicator.dispatchErrorTooLong(length);
					} else if (unlikely(address + length > MAX_STORAGE_ADDRESS)) {
						communicator.dispatchErrorOutOfRange(address, length);
					} else {
						uint8_t buffer[length];
						conf.readBytes(address, length, buffer);
						communicator.dispatchReadStorageResult(address, length, buffer);
					}
				}
				break;
			case CMD_REBOOT:
				for (;;); // block the thread and let WDT triggers an reset
			default:
				communicator.dispatchErrorUnknownCommand(messenger.commandID());
		}
		#if defined(DEBUG_SERIAL)
		t2 = micros();
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",cmd handler took "));
		DEBUG_SERIAL.print(t2 - t1);
		DEBUG_SERIAL.print("us;");
		#endif
	});

	communicator.dispatchBoot();

	#if defined(DEBUG_SERIAL)
	t2 = micros();
	DEBUG_SERIAL.print((int)EVT_DEBUG);
	DEBUG_SERIAL.print(F(",setup() took "));
	DEBUG_SERIAL.print(t2 - t1);
	DEBUG_SERIAL.print("us;");
	#endif
}

void loop() {
	#if defined(DEBUG_SERIAL)
	static uint32_t last_millis = millis();
	uint32_t t1, t2;
	t1 = micros();
	#endif

	wdt_reset(); // feed the dog

	// read key states
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi::receive();
    in.bytes[1] = spi::receive();
    in.bytes[2] = spi::receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros();

	// check the timeout tracker before we feed the debouncers, since debouncers
	// might trigger tracker.start() when a coin is confirmed.
	if (TRACKER_NACK.trigger(now))
	{
		TRACKER_EJECT.stop();
		TRACKER_TICKET.stop();
		out.port.ssr1 = false;
		out.port.ssr2 = false;
		do_send = true;
	}
	if (TRACKER_EJECT.trigger(now))
	{
		uint8_t const coins = conf.getCoinsToEject(TRACK_EJECT);
		if (coins) {
			communicator.dispatchErrorEjectTimeout(TRACK_EJECT, coins);
			out.port.ssr1 = false;
			do_send = true;
		}
	}
	if (TRACKER_TICKET.trigger(now))
	{
		uint8_t const coins = conf.getCoinsToEject(TRACK_TICKET);
		if (coins) {
			communicator.dispatchErrorEjectTimeout(TRACK_TICKET, coins);
			out.port.ssr2 = false;
			do_send = true;
		}
	}

	// debounce the inputs
	const Configuration::TrackLevelsT &track_levels = conf.getTrackLevels();
	debounce_insert_1.feed(in.port.sw12, track_levels.bits.track_level_2, now);
	debounce_insert_2.feed(in.port.sw13, track_levels.bits.track_level_3, now);
	debounce_banknote.feed(in.port.sw20, track_levels.bits.track_level_4, now);
	debounce_eject.feed(in.port.sw11, track_levels.bits.track_level_0, now);
	debounce_ticket.feed(in.port.sw14, track_levels.bits.track_level_1, now);

	// pulse the counters
	if (pulse_counters[COUNTER_SCORE].update(now))
	{
		out.port.counter1 = pulse_counters[COUNTER_SCORE].get();
		do_send = true;
	}
	if (pulse_counters[COUNTER_WASH].update(now))
	{
		out.port.counter2 = pulse_counters[COUNTER_WASH].get();
		do_send = true;
	}
	if (pulse_counters[COUNTER_INSERT].update(now))
	{
		out.port.counter3 = pulse_counters[COUNTER_INSERT].get();
		do_send = true;
	}
	if (pulse_counters[COUNTER_EJECT].update(now))
	{
		out.port.counter4 = pulse_counters[COUNTER_EJECT].get();
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
		communicator.dispatchKeysResult(3, masked);

		previous_in.bytes[0] = masked[0];
		previous_in.bytes[1] = masked[1];
		previous_in.bytes[2] = masked[2];
	}

	// feed the serial data before we send, because messenger might want to
	// modify stuff.
	messenger.feedinSerialData();

	// send the outputs only when needed
	#if defined(DEBUG_SERIAL)
	if (do_send || millis() - last_millis > 1000) {
	#endif
		if (do_send) {
			do_send = false;
	        fastDigitalWrite(PIN_LATCH_OUT, LOW);
	        spi::send(out.bytes[0]);
	        spi::send(out.bytes[1]);
	        spi::send(out.bytes[2]);
	        fastDigitalWrite(PIN_LATCH_OUT, HIGH);
		}

	#if defined(DEBUG_SERIAL)
		t2 = micros();
		last_millis = millis();
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",loop() took "));
		DEBUG_SERIAL.print(t2 - t1);
		DEBUG_SERIAL.print("us;");
    }
	#endif
}
