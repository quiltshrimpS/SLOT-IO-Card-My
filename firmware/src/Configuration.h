#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <Arduino.h>
#include <Wire.h>
#include <utility/twi.h>

#include <FRAM_MB85RC_I2C.h>

#include <util/crc16.h>

#include "Communication.h"

#define NUM_EJECT_TRACKS				(1)
#define NUM_INSERT_TRACKS				(4)
#define NUM_TRACKS						(NUM_EJECT_TRACKS + NUM_INSERT_TRACKS)

#define CONF_OFFSET_COIN_TRACK_LEVEL	(0x0000)
#define CONF_SIZE_COIN_TRACK_LEVEL		(1)
#define CONF_OFFSET_COINS_TO_EJECT		(CONF_OFFSET_COIN_TRACK_LEVEL + CONF_SIZE_COIN_TRACK_LEVEL)
#define CONF_SIZE_COINS_TO_EJECT		(NUM_EJECT_TRACKS * sizeof(uint8_t))
#define CONF_OFFSET_COIN_COUNT			(CONF_OFFSET_COINS_TO_EJECT + CONF_SIZE_COINS_TO_EJECT)
#define CONF_SIZE_COIN_COUNT			(NUM_TRACKS * sizeof(uint32_t))
#define CONF_OFFSET_EJECT_TIMEOUT		(CONF_OFFSET_COIN_COUNT + CONF_SIZE_COIN_COUNT)
#define CONF_SIZE_EJECT_TIMEOUT			(NUM_EJECT_TRACKS * sizeof(uint32_t))
#define CONF_OFFSET_CHECKSUM			(CONF_OFFSET_EJECT_TIMEOUT + CONF_SIZE_EJECT_TIMEOUT)
#define CONF_SIZE_CHECKSUM				(1)
#define CONF_OFFSET_END					(CONF_OFFSET_CHECKSUM + CONF_SIZE_CHECKSUM)
#define CONF_SIZE_ALL					(CONF_OFFSET_END)

#define CONF_ADDR_BEGIN					(0x0000)
#define CONF_ADDR_BANK_0				(CONF_ADDR_BEGIN)
#define CONF_ADDR_BANK_1				(CONF_ADDR_BEGIN + 0x0100)

#define TRACK_EJECT			(0)
#define TRACK_INSERT_1		(1)
#define TRACK_INSERT_2		(2)
#define TRACK_INSERT_3		(3)
#define TRACK_BANKNOTE		(4)
#define TRACK_NOT_A_TRACK	(0xFF)

#define TRACK_LEVELS_DEFAULT	(0b11111110)
#define EJECT_TIMEOUT_DEFAULT	(10000000L) // us

#define MAX_BYTES_LENGTH	(64)

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
		//   - crc: crc8_ccitt of all data bytes with seed `0x87` (0x87 is
		//          choosen because that's how `42` is choosen...)
		//
		// this class will try to use the one with good `crc`

		// read the data from bank 0
		readBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
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
		readBytes(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
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

			writeBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			writeBytes(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
		} else {
			if (bank0_checksum_good) {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.print((int)EVT_DEBUG);
				DEBUG_SERIAL.print(F(",bank1 bad/, use bank0;"));
				#endif
				// bank1 is bad, read back bank0
				readBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			} else /* if (bank1_checksum_good) */ {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.print((int)EVT_DEBUG);
				DEBUG_SERIAL.print(F(",bank0 bad/, use bank1;"));
				#endif
				// bank0 is bad, use bank1.
			}
		}

		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
	}

	__attribute__((always_inline)) inline
	uint8_t getTrackLevel(uint8_t const track) {
		return bitRead(_data.configs.track_levels.bytes, track);
	}

	__attribute__((always_inline)) inline
	void setTrackLevel(uint8_t const track, bool const level) {
		bitSet(_data.configs.track_levels.bytes, track);
		_data.configs.crc = _getChecksum();
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_COIN_TRACK_LEVEL, _data.bytes[CONF_OFFSET_COIN_TRACK_LEVEL]);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_COIN_TRACK_LEVEL, _data.bytes[CONF_OFFSET_COIN_TRACK_LEVEL]);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
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
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_COINS_TO_EJECT + track, coins);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_COINS_TO_EJECT + track, coins);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
	}

	__attribute__((always_inline)) inline
	uint32_t getCoinCount(uint8_t const track) {
		return _data.configs.coin_count[track];
	}

	__attribute__((always_inline)) inline
	void setCoinCount(uint8_t const track, uint32_t const count) {
		_data.configs.coin_count[track] = count;
		_data.configs.crc = _getChecksum();
		_fram.writeLong(CONF_ADDR_BANK_0 + CONF_OFFSET_COIN_COUNT + track * sizeof(uint32_t), count);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		_fram.writeLong(CONF_ADDR_BANK_1 + CONF_OFFSET_COIN_COUNT + track * sizeof(uint32_t), count);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
	}

	__attribute__((always_inline)) inline
	uint32_t getEjectTimeout(uint8_t const track) {
		return _data.configs.eject_timeout[track];
	}

	__attribute__((always_inline)) inline
	void setEjectTimeout(uint8_t const track, uint32_t const timeout) {
		_data.configs.eject_timeout[track] = timeout;
		_data.configs.crc = _getChecksum();
		_fram.writeLong(CONF_ADDR_BANK_0 + CONF_OFFSET_EJECT_TIMEOUT + track * sizeof(uint32_t), timeout);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		_fram.writeLong(CONF_ADDR_BANK_1 + CONF_OFFSET_EJECT_TIMEOUT + track * sizeof(uint32_t), timeout);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.crc);
		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
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
	void readBytes(uint16_t addr, uint8_t length, uint8_t * buffer) {
		while (length != 0) {
			uint8_t bytes = min(length, TWI_BUFFER_LENGTH);
			_fram.readArray(addr, bytes, buffer);
			addr += bytes;
			buffer += bytes;
			length -= bytes;
		}
	}

	__attribute__((always_inline)) inline
	void writeBytes(uint16_t addr, uint8_t length, uint8_t * buffer) {
		while (length != 0) {
			uint8_t bytes = min(length, TWI_BUFFER_LENGTH);
			_fram.writeArray(addr, bytes, buffer);
			addr += bytes;
			buffer += bytes;
			length -= bytes;
		}
	}

private:
	__attribute__((always_inline, optimize("unroll-loops"))) inline
	uint8_t _getChecksum() {
		#if defined(DEBUG_SERIAL)
		uint32_t t1, t2;
		t1 = micros();
		#endif

		uint8_t crc = 0x87; // randomly picked seed...
		for (uint8_t i = 0;i < CONF_SIZE_ALL - 1;++i)
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

	union {
		uint8_t bytes[CONF_SIZE_ALL];
		struct {
			union TrackLevelsT track_levels;

			uint8_t coins_to_eject[NUM_EJECT_TRACKS];
			uint32_t coin_count[NUM_TRACKS];
			uint32_t eject_timeout[NUM_EJECT_TRACKS];

			uint8_t crc;
		} configs;
	} _data;

	FRAM_MB85RC_I2C _fram;
};

#endif
