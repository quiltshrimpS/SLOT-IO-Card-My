#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <Arduino.h>
#include <Wire.h>
#include <utility/twi.h>
#include <util/crc16.h>

#include <FRAM_MB85RC_I2C.h>

#include "Communication.h"

#define NUM_EJECT_TRACKS				(2)
#define NUM_INSERT_TRACKS				(3)
#define NUM_TRACKS						(NUM_EJECT_TRACKS + NUM_INSERT_TRACKS)
#define NUM_COUNTERS					(4)

// change this when configuration layout changes.
#define CONF_VERSION					(0x02)

#define CONF_ADDR_BEGIN					(0x0000)
#define CONF_ADDR_BANK_0				(CONF_ADDR_BEGIN)
#define CONF_ADDR_BANK_1				(CONF_ADDR_BANK_0 + 0x0100)
#define CONF_ADDR_USER_BEGIN			(CONF_ADDR_BANK_1 + 0x0100)

#define TRACK_EJECT				(0)
#define TRACK_TICKET			(1)
#define TRACK_INSERT_1			(2)
#define TRACK_INSERT_2			(3)
#define TRACK_BANKNOTE			(4)
#define TRACK_NOT_A_TRACK		(0xFF)

#define COUNTER_SCORE			(0)
#define COUNTER_WASH			(1)
#define COUNTER_INSERT			(2)
#define COUNTER_EJECT			(3)
#define COUNTER_NOT_A_COUNTER	(0xFF)

//                             Eject -----+
//                            Ticket ----+|
//                          Insert 1 ---+||
//                          Insert 2 --+|||
//                          Banknote -+||||
//                                    |||||
#define TRACK_LEVELS_DEFAULT	(0b11111110)

#define EJECT_TIMEOUT_DEFAULT	(10000000L) // us

#define MAX_BYTES_LENGTH		(64)
#define MAX_STORAGE_ADDRESS		(16384u)

class Configuration {
public:
	union TrackLevelsT {
		struct {
			uint8_t track_level_0:1;
			uint8_t track_level_1:1;
			uint8_t track_level_2:1;
			uint8_t track_level_3:1;
			uint8_t track_level_4:1;
			uint8_t track_level_5:1;
			uint8_t track_level_6:1;
			uint8_t track_level_7:1;
		} bits;
		uint8_t bytes;
	};

	Configuration():
		_fram(MB85RC_DEFAULT_ADDRESS, true, /* WP */ A7, 16 /* kb */)
	{
	}

	__attribute__((always_inline)) inline
	void begin() {
		// setup underlying facilities
		Wire.begin();
		Wire.setClock(TWI_BAUDRATE); // set clock after Wire.begin()
		_fram.begin();

		// there are 2 banks of memories inside the fram:
		//   - CONF_ADDR_BANK_0 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		//   - CONF_ADDR_BANK_1 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		// the last byte of the bank is the `crc`.
		//   - crc: crc8_ccitt of all data bytes with `CONF_VERSION` as the seed.
		//
		// this class will try to use the one with good `crc`

		// read the data from bank 0
		_fram.readFrom(CONF_ADDR_BANK_0, _data);
		uint8_t crc0 = _getChecksum();
		bool bank0_checksum_good = crc0 == _data.configs.crc;

		#if defined(DEBUG_SERIAL)
		dumpBuffer("bank0", _data.bytes, CONF_SIZE_ALL);
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",bank0: crc = "));
		if (crc0 < 0x10)
			DEBUG_SERIAL.print('0');
		DEBUG_SERIAL.print((int)crc0, HEX);
		DEBUG_SERIAL.print(';');
		#endif

		// read the data from bank 1
		_fram.readFrom(CONF_ADDR_BANK_1, _data);
		uint8_t crc1 = _getChecksum();
		bool bank1_checksum_good = crc1 == _data.configs.crc;

		#if defined(DEBUG_SERIAL)
		dumpBuffer("bank1", _data.bytes, CONF_SIZE_ALL);
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",bank1: crc = "));
		if (crc1 < 0x10)
			DEBUG_SERIAL.print('0');
		DEBUG_SERIAL.print((int)crc1, HEX);
		DEBUG_SERIAL.print(';');
		#endif

		// decide which bank to use
		if (bank0_checksum_good && bank1_checksum_good) {
			// both checksum good, does nothing.
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.print((int)EVT_DEBUG);
			DEBUG_SERIAL.print(F(",both bank good/, use bank1;"));
			#endif
		} else if (!bank0_checksum_good && !bank1_checksum_good) {
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.print((int)EVT_DEBUG);
			DEBUG_SERIAL.print(F(",both bank bad/, initializing...;"));
			#endif
			// both bad, initialize bank0 and use it, write back to both bank

			for (uint8_t i = 0;i < NUM_EJECT_TRACKS;++i) {
				_data.configs.coins_to_eject[i] = 0;
				_data.configs.eject_timeout[i] = EJECT_TIMEOUT_DEFAULT;
			}
			for (uint8_t i = 0;i < NUM_TRACKS;++i)
				_data.configs.coin_count[i] = 0;

			_data.configs.track_levels.bytes = TRACK_LEVELS_DEFAULT;
			_data.configs.crc = _getChecksum();

			_fram.writeTo(CONF_ADDR_BANK_0, _data);
			_fram.writeTo(CONF_ADDR_BANK_1, _data);
		} else {
			if (bank0_checksum_good) {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.print((int)EVT_DEBUG);
				DEBUG_SERIAL.print(F(",bank1 bad/, use bank0;"));
				#endif
				// bank1 is bad, read back bank0
				readBytes(CONF_ADDR_BANK_0, sizeof(ConfigDataT), _data.bytes);
			} else /* if (bank1_checksum_good) */ {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.print((int)EVT_DEBUG);
				DEBUG_SERIAL.print(F(",bank0 bad/, use bank1;"));
				#endif
				// bank0 is bad, use bank1.
			}
		}

		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	uint8_t getTrackLevel(uint8_t const track) {
		return bitRead(_data.configs.track_levels.bytes, track);
	}

	__attribute__((always_inline)) inline
	void setTrackLevel(uint8_t const track, bool const level) {
		bitSet(_data.configs.track_levels.bytes, track);
		_data.configs.crc = _getChecksum();
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.track_levels) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.track_levels);
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.track_levels) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.track_levels);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	union TrackLevelsT & getTrackLevels() {
		return _data.configs.track_levels;
	}

	__attribute__((always_inline)) inline
	uint8_t getCoinsToEject(uint8_t const track) {
		return _data.configs.coins_to_eject[track];
	}

	__attribute__((always_inline)) inline
	void setCoinsToEject(uint8_t const track, uint8_t const coins) {
		_data.configs.coins_to_eject[track] = coins;
		_data.configs.crc = _getChecksum();
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.coins_to_eject[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), coins);
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.coins_to_eject[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), coins);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	uint32_t getCoinCount(uint8_t const track) {
		return _data.configs.coin_count[track];
	}

	__attribute__((always_inline)) inline
	void setCoinCount(uint8_t const track, uint32_t const & count) {
		_data.configs.coin_count[track] = count;
		_data.configs.crc = _getChecksum();
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.coin_count[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), count);
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.coin_count[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), count);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	uint32_t getEjectTimeout(uint8_t const track) {
		return _data.configs.eject_timeout[track];
	}

	__attribute__((always_inline)) inline
	void setEjectTimeout(uint8_t const track, uint32_t const & timeout) {
		_data.configs.eject_timeout[track] = timeout;
		_data.configs.crc = _getChecksum();
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.eject_timeout[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), timeout);
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.eject_timeout[track])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), timeout);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	uint32_t getCounterTicks(uint8_t const counter) {
		return _data.configs.counter_ticks[counter];
	}

	__attribute__((always_inline)) inline
	void setCounterTicks(uint8_t const counter, uint32_t const ticks) {
		_data.configs.counter_ticks[counter] = ticks;
		_data.configs.crc = _getChecksum();
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.counter_ticks[counter])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), ticks);
		_fram.writeTo(CONF_ADDR_BANK_0 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&(_data.configs.counter_ticks[counter])) - reinterpret_cast<uint8_t const * const>(&_data.configs)), ticks);
		_fram.writeTo(CONF_ADDR_BANK_1 + (reinterpret_cast<uint8_t const * const>(&_data.configs.crc) - reinterpret_cast<uint8_t const * const>(&_data.configs)), _data.configs.crc);
		dumpBuffer("_data", _data.bytes, sizeof(ConfigDataT));
	}

	__attribute__((always_inline)) inline
	void dumpBuffer(char const * const tag, uint8_t const * const buffer, size_t const size) {
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",conf"));
		if (tag != nullptr) {
			DEBUG_SERIAL.print(' ');
			DEBUG_SERIAL.print(tag);
		}
		DEBUG_SERIAL.print(':');
		for (size_t i = 0;i < size;++i) {
			DEBUG_SERIAL.print(' ');
			if (buffer[i] < 0x10)
				DEBUG_SERIAL.print('0');
			DEBUG_SERIAL.print((int)(buffer[i]), HEX);
		}
		DEBUG_SERIAL.print(';');
		#endif
	}

	__attribute__((always_inline)) inline
	void readBytes(uint16_t const & addr, uint8_t const length, uint8_t * const buffer) {
		_fram.readArray(addr, length, buffer);
	}

	__attribute__((always_inline)) inline
	void writeBytes(uint16_t const & addr, uint8_t const length, uint8_t const * const buffer) {
		_fram.writeArray(addr, length, buffer);
	}

private:
	__attribute__((always_inline, optimize("unroll-loops"))) inline
	uint8_t _getChecksum() {
		#if defined(DEBUG_SERIAL)
		uint32_t t1, t2;
		t1 = micros();
		#endif

		uint8_t crc = CONF_VERSION;
		for (uint8_t i = 0;i < sizeof(ConfigDataT) - sizeof(_data.configs.crc);++i)
			crc = _crc8_ccitt_update(crc, _data.bytes[i]);

		#if defined(DEBUG_SERIAL)
		t2 = micros();
		DEBUG_SERIAL.print((int)EVT_DEBUG);
		DEBUG_SERIAL.print(F(",CRC took "));
		DEBUG_SERIAL.print(t2 - t1);
		DEBUG_SERIAL.print("us;");
		#endif
		return crc;
	}

	struct ConfigDataT {
		union TrackLevelsT track_levels;

		uint8_t coins_to_eject[NUM_EJECT_TRACKS];
		uint32_t coin_count[NUM_TRACKS];
		uint32_t eject_timeout[NUM_EJECT_TRACKS];
		uint32_t counter_ticks[NUM_COUNTERS];

		uint8_t crc;
	};

	union {
		uint8_t bytes[sizeof(ConfigDataT)];
		struct ConfigDataT configs;
	} _data;

	// FIXME: hardware layout connects WP to A7, but A7 can only be used as ADC
	//        input and not digital output, so we have to leave WP unmanaged.
	FRAM_MB85RC_I2C_T<WriteProtect_Unmanaged> _fram;
};

#endif
