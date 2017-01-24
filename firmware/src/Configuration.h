#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <Arduino.h>
#include <Wire.h>
#include <utility/twi.h>
#include <FRAM_MB85RC_I2C.h>

#define CONF_OFFSET_COIN_TRACK_LEVEL	(0x0000)
#define CONF_SIZE_COIN_TRACK_LEVEL		(1)
#define CONF_OFFSET_COINS_TO_EJECT		(CONF_OFFSET_COIN_TRACK_LEVEL + CONF_SIZE_COIN_TRACK_LEVEL)
#define CONF_SIZE_COINS_TO_EJECT		(8 * sizeof(uint8_t))
#define CONF_OFFSET_COIN_COUNT			(CONF_OFFSET_COINS_TO_EJECT + CONF_SIZE_COINS_TO_EJECT)
#define CONF_SIZE_COIN_COUNT			(8 * sizeof(uint32_t))
#define CONF_OFFSET_CHECKSUM			(CONF_OFFSET_COIN_COUNT + CONF_SIZE_COIN_COUNT)
#define CONF_SIZE_CHECKSUM				(1)
#define CONF_OFFSET_END					(CONF_OFFSET_CHECKSUM + CONF_SIZE_CHECKSUM)
#define CONF_SIZE_ALL					(CONF_OFFSET_END)

#define CONF_ADDR_BEGIN					(0x0000)
#define CONF_ADDR_BANK_0				(CONF_ADDR_BEGIN)
#define CONF_ADDR_BANK_1				(CONF_ADDR_BEGIN + 0x0100)

#define TRACK_INSERT_1		(0x00)
#define TRACK_INSERT_2		(0x01)
#define TRACK_INSERT_3		(0x02)
#define TRACK_BANKNOTE		(0x80)
#define TRACK_EJECT			(0xC0)
#define TRACK_NOT_A_TRACK	(0xFF)

#define TRACK_LEVELS_DEFAULT	(0b11101111)
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

	Configuration(FRAM_MB85RC_I2C &fram):
		_fram(fram)
	{
	}

	__attribute__((always_inline)) inline
	void begin() {
		// there are 2 banks of memories inside the fram:
		//   - CONF_ADDR_BANK_0 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		//   - CONF_ADDR_BANK_1 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		// the last byte of the bank is the `checksum`.
		//   - checksum: every byte (including `seq`) except the `checksum`
		//               byte XOR-ed togeter with `0x87` (0x87 is choosen because
		//               that's how `42` is choosen...)
		//
		// this class will try to use the one with good `checksum` and newer `seq`

		// read the data from bank 0
		readBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
		uint8_t checksum_0 = _getChecksum();
		bool bank0_checksum_good = checksum_0 == _data.configs.checksum;

		dumpBuffer("bank0", _data.bytes, CONF_SIZE_ALL);
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("Configuration bank0: checksum = "));
		if (checksum_0 < 0x10)
			DEBUG_SERIAL.print('0');
		DEBUG_SERIAL.println((int)checksum_0, HEX);
		#endif

		// read the data from bank 1
		readBytes(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
		uint8_t checksum_1 = _getChecksum();
		bool bank1_checksum_good = checksum_1 == _data.configs.checksum;

		dumpBuffer("bank1", _data.bytes, CONF_SIZE_ALL);
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("Configuration bank1: checksum = "));
		if (checksum_1 < 0x10)
			DEBUG_SERIAL.print('0');
		DEBUG_SERIAL.println((int)checksum_1, HEX);
		#endif

		// decide which bank to use
		if (bank0_checksum_good && bank1_checksum_good) {
			// both checksum good, does nothing.
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.println(F("Configuration: both good, use bank 1."));
			#endif
		} else if (!bank0_checksum_good && !bank1_checksum_good) {
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.println(F("Configuration: both bad, initialize bank 0 and use it."));
			#endif
			// both bad, initialize bank0 and use it, write back to both bank
			memset(_data.bytes, 0, CONF_SIZE_ALL);
			_data.configs.track_levels.bytes = TRACK_LEVELS_DEFAULT;
			_data.configs.eject_timeout[0] = EJECT_TIMEOUT_DEFAULT;
			_data.configs.eject_timeout[1] = EJECT_TIMEOUT_DEFAULT;
			_data.configs.eject_timeout[2] = EJECT_TIMEOUT_DEFAULT;
			_data.configs.eject_timeout[3] = EJECT_TIMEOUT_DEFAULT;
			_data.configs.checksum = _getChecksum();
			writeBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			writeBytes(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
		} else {
			if (bank0_checksum_good) {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println(F("Configuration: bank 1 bad, use bank 0."));
				#endif
				// bank1 is bad, read back bank0
				readBytes(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			} else /* if (bank1_checksum_good) */ {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println(F("Configuration: bank 0 bad, use bank 1."));
				#endif
				// bank0 is bad, use bank1.
			}
		}

		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
	}

	__attribute__((always_inline)) inline
	uint8_t getTrackLevel(uint8_t track) {
		if (track == TRACK_EJECT) // eject track
			return _data.configs.track_levels.bits.track_level_4;
		if (track == TRACK_INSERT_1) // coin track 1
			return _data.configs.track_levels.bits.track_level_0;
		if (track == TRACK_INSERT_2) // coin track 2
			return _data.configs.track_levels.bits.track_level_1;
		if (track == TRACK_INSERT_3) // coin track 3
			return _data.configs.track_levels.bits.track_level_2;
		if (track == TRACK_BANKNOTE) // banknote track
			return _data.configs.track_levels.bits.track_level_3;
		return TRACK_NOT_A_TRACK;
	}

	__attribute__((always_inline)) inline
	bool setTrackLevel(uint8_t track, bool level) {
		if (track == TRACK_EJECT) // eject track
			_data.configs.track_levels.bits.track_level_4 = level;
		else if (track == TRACK_INSERT_1) // coin track 1
			_data.configs.track_levels.bits.track_level_0 = level;
		else if (track == TRACK_INSERT_2) // coin track 2
			_data.configs.track_levels.bits.track_level_1 = level;
		else if (track == TRACK_INSERT_3) // coin track 3
			_data.configs.track_levels.bits.track_level_2 = level;
		else if (track == TRACK_BANKNOTE) // banknote track
			_data.configs.track_levels.bits.track_level_3 = level;
		else
			return false;

		_data.configs.checksum = _getChecksum();
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_COIN_TRACK_LEVEL, _data.bytes[CONF_OFFSET_COIN_TRACK_LEVEL]);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_COIN_TRACK_LEVEL, _data.bytes[CONF_OFFSET_COIN_TRACK_LEVEL]);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);
		return true;
	}

	__attribute__((always_inline)) inline
	uint8_t getCoinsToEject(uint8_t track) {
		if (track == TRACK_EJECT)
			return _data.configs.coins_to_eject[4];
		return 0;
	}

	__attribute__((always_inline)) inline
	bool setCoinsToEject(uint8_t track, uint8_t coins) {
		uint8_t track_idx = 0xFF;
		if (track == TRACK_EJECT)
			track_idx = 4;
		else
			return false;

		_data.configs.coins_to_eject[track_idx] = coins;
		_data.configs.checksum = _getChecksum();
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_COINS_TO_EJECT + track_idx, coins);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_COINS_TO_EJECT + track_idx, coins);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);

		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);

		return true;
	}

	__attribute__((always_inline)) inline
	uint32_t getCoinCount(uint8_t track) {
		if (track == TRACK_EJECT)
			return _data.configs.coin_count[4];
		else if (track == TRACK_INSERT_1)
			return _data.configs.coin_count[0];
		else if (track == TRACK_INSERT_2)
			return _data.configs.coin_count[1];
		else if (track == TRACK_INSERT_3)
			return _data.configs.coin_count[2];
		else if (track == TRACK_BANKNOTE)
			return _data.configs.coin_count[3];
		return 0;
	}

	__attribute__((always_inline)) inline
	bool setCoinCount(uint8_t track, uint32_t count) {
		uint8_t track_idx = 0xFF;
		if (track == TRACK_EJECT)
			track_idx = 4;
		else if (track == TRACK_INSERT_1)
			track_idx = 0;
		else if (track == TRACK_INSERT_2)
			track_idx = 1;
		else if (track == TRACK_INSERT_3)
			track_idx = 2;
		else if (track == TRACK_BANKNOTE)
			track_idx = 3;
		else
		 	return false;

		_data.configs.coin_count[track_idx] = count;
		_data.configs.checksum = _getChecksum();
		_fram.writeLong(CONF_ADDR_BANK_0 + CONF_OFFSET_COIN_COUNT + track_idx * sizeof(uint32_t), count);
		_fram.writeByte(CONF_ADDR_BANK_0 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		_fram.writeLong(CONF_ADDR_BANK_1 + CONF_OFFSET_COIN_COUNT + track_idx * sizeof(uint32_t), count);
		_fram.writeByte(CONF_ADDR_BANK_1 + CONF_OFFSET_CHECKSUM, _data.configs.checksum);

		dumpBuffer("_data", _data.bytes, CONF_SIZE_ALL);

		return true;
	}

	__attribute__((always_inline)) inline
	void dumpBuffer(char const * const tag, uint8_t const * const buffer, size_t const size) {
		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print(F("Configuration"));
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
		DEBUG_SERIAL.println();
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
	__attribute__((always_inline)) inline
	uint8_t _getChecksum() {
		uint8_t checksum = 0x87; // randomly picked seed...
		for (uint8_t i = 0;i < CONF_SIZE_ALL - 1;++i)
			checksum ^= _data.bytes[i];
		return checksum;
	}

	union {
		uint8_t bytes[CONF_SIZE_ALL];
		struct {
			union TrackLevelsT track_levels;

			uint8_t coins_to_eject[8];
			uint32_t coin_count[8];

			uint8_t checksum;
		} configs;
	} _data;
	FRAM_MB85RC_I2C & _fram;
};

#endif
