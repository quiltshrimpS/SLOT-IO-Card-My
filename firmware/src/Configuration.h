#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <Arduino.h>
#include <FRAM_MB85RC_I2C.h>

#define CONF_OFFSET_SEQ					(0x0000)
#define CONF_SIZE_SEQ					(1)
#define CONF_OFFSET_COIN_TRACK_LEVEL	(CONF_OFFSET_SEQ + CONF_SIZE_SEQ)
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

#define TRACK_EJECT		(0x80)
#define TRACK_INSERT_1	(0x00)
#define TRACK_INSERT_2	(0x01)
#define TRACK_INSERT_3	(0x02)
#define TRACK_BANKNOTE	(0x40)

#define ERR_NOT_A_TRACK	(0xFF)

class Configuration {
public:
	Configuration(FRAM_MB85RC_I2C &fram):
		_bank(false),
		_fram(fram)
	{
	}

	void begin() {
		// there are 2 banks of memories inside the fram:
		//   - CONF_ADDR_BANK_0 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		//   - CONF_ADDR_BANK_1 (256 bytes): [ CONF_SIZE_ALL ] [ RESERVED ]
		// the first byte of the bank is the `seq`, and last byte is the `checksum`.
		//   - seq: 0x00 -> 0x01 -> 0x02 -> ... -> 0xFE -> 0xFF -> 0x00, and starts over
		//   - checksum: every byte (including `seq`) except the `checksum`
		//               byte XOR-ed togeter with `0x87` (0x87 is choosen because
		//               that's how `42` is choosen...)
		//
		// this class will try to use the one with good `checksum` and newer `seq`

		// read the data from bank 0
		_fram.readArray(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
		bool bank0_checksum_good = _getChecksum() == _data.configs.checksum;
		uint8_t bank0_seq = _data.configs.seq + 1;

		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print("Configuration: (bank 0)");
		for (int i = 0;i < CONF_SIZE_ALL;++i) {
			DEBUG_SERIAL.print(' ');
			if (_data.bytes[i] < 0x10)
				DEBUG_SERIAL.print('0');
			DEBUG_SERIAL.print((int)(_data.bytes[i]), HEX);
		}
		DEBUG_SERIAL.println();
		#endif

		// read the data from bank 1
		_fram.readArray(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
		bool bank1_checksum_good = _getChecksum() == _data.configs.checksum;

		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print("Configuration: (bank 1)");
		for (int i = 0;i < CONF_SIZE_ALL;++i) {
			DEBUG_SERIAL.print(' ');
			if (_data.bytes[i] < 0x10)
				DEBUG_SERIAL.print('0');
			DEBUG_SERIAL.print((int)(_data.bytes[i]), HEX);
		}
		DEBUG_SERIAL.println();
		#endif

		// decide which bank to use
		if (bank0_checksum_good && bank1_checksum_good) {
			// both checksum good, compare seq
			if (bank0_seq == _data.configs.seq) {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println("Configuration: both good, using bank 1");
				#endif
				// bank0.seq + 1 == bank1.seq, bank1 is newer
				// nothing to be done, _data is already bank1.
				_bank = true;
			} else {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println("Configuration: both good, using bank 0");
				#endif
				// bank0 is newer, read them back from fram
				_fram.readArray(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			}
		} else if (!bank0_checksum_good && !bank1_checksum_good) {
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.println("Configuration: both bad, initialize bank 0 and use it.");
			#endif
			// both bad, initialize bank0 and use it, write back to both bank
			memset(_data.bytes, 0, CONF_SIZE_ALL);
			_data.configs.checksum = _getChecksum();
			_fram.writeArray(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			_fram.writeArray(CONF_ADDR_BANK_1, CONF_SIZE_ALL, _data.bytes);
		} else {
			if (bank0_checksum_good) {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println("Configuration: bank 1 bad, use bank 0.");
				#endif
				// bank1 is bad, read back bank0
				_fram.readArray(CONF_ADDR_BANK_0, CONF_SIZE_ALL, _data.bytes);
			} else /* if (bank1_checksum_good) */ {
				#if defined(DEBUG_SERIAL)
				DEBUG_SERIAL.println("Configuration: bank 0 bad, use bank 1.");
				#endif
				// bank0 is bad, use bank1.
				_bank = true;
			}
		}

		#if defined(DEBUG_SERIAL)
		DEBUG_SERIAL.print("Configuration: (bank ");
		DEBUG_SERIAL.print(_bank ? "0)" : "1)");
		for (int i = 0;i < CONF_SIZE_ALL;++i) {
			DEBUG_SERIAL.print(' ');
			if (_data.bytes[i] < 0x10)
				DEBUG_SERIAL.print('0');
			DEBUG_SERIAL.print((int)(_data.bytes[i]), HEX);
		}
		DEBUG_SERIAL.println();
		#endif
	}

	uint8_t getTrackLevel(uint8_t track) {
		if (track == TRACK_EJECT) // eject track
			return _data.configs.track_level_4;
		if (track == TRACK_INSERT_1) // coin track 1
			return _data.configs.track_level_0;
		if (track == TRACK_INSERT_2) // coin track 2
			return _data.configs.track_level_1;
		if (track == TRACK_INSERT_3) // coin track 3
			return _data.configs.track_level_2;
		if (track == TRACK_BANKNOTE) // banknote track
			return _data.configs.track_level_3;
		return ERR_NOT_A_TRACK;
	}

	void setTrackLevel(uint8_t track, bool level) {
		if (track == TRACK_EJECT) // eject track
			_data.configs.track_level_4 = level;
		else if (track == TRACK_INSERT_1) // coin track 1
			_data.configs.track_level_0 = level;
		else if (track == TRACK_INSERT_2) // coin track 2
			_data.configs.track_level_1 = level;
		else if (track == TRACK_INSERT_3) // coin track 3
			_data.configs.track_level_2 = level;
		else if (track == TRACK_BANKNOTE) // banknote track
			_data.configs.track_level_3 = level;

		_bank = !_bank;
		_data.configs.checksum = _getChecksum();
		_fram.writeByte((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_COIN_TRACK_LEVEL, _data.bytes[CONF_OFFSET_COIN_TRACK_LEVEL]);
		_fram.writeByte((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
	}

	uint8_t getCoinsToEject(uint8_t track) {
		uint8_t track_idx = 0xFF;
		if (track == TRACK_EJECT)
			track_idx = 4;

		return track_idx == 0xFF ? 0 : _data.configs.coins_to_eject[track_idx];
	}

	void setCoinsToEject(uint8_t track, uint8_t coins) {
		uint8_t track_idx = 0xFF;
		if (track == TRACK_EJECT)
			track_idx = 4;

		if (track_idx != 0xFF) {
			_data.configs.coins_to_eject[track_idx] = coins;
			_bank = !_bank;
			_data.configs.checksum = _getChecksum();
			_fram.writeByte((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_COINS_TO_EJECT + track_idx, coins);
			_fram.writeByte((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		}
	}

	uint32_t getCoinCount(uint8_t track) {
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

		return track_idx == 0xFF ? 0 : _data.configs.coin_count[track_idx];
	}

	void setCoinCount(uint8_t track, uint32_t count) {
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

		if (track_idx != 0xFF) {
			_data.configs.coin_count[track_idx] = count;
			_bank = !_bank;
			_data.configs.checksum = _getChecksum();
			_fram.writeLong((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_COIN_COUNT + track_idx, count);
			_fram.writeByte((_bank ? CONF_ADDR_BANK_0 : CONF_ADDR_BANK_1) + CONF_OFFSET_CHECKSUM, _data.configs.checksum);
		}
	}

private:
	uint8_t _getChecksum() {
		uint8_t checksum = 0x87; // randomly picked seed...
		for (int i = 0;i < CONF_SIZE_ALL - 1;++i)
			checksum ^= _data.bytes[i];
		return checksum;
	}

	union {
		uint8_t bytes[CONF_SIZE_ALL];
		struct {
			uint8_t seq;

			uint8_t track_level_0:1;
			uint8_t track_level_1:1;
			uint8_t track_level_2:1;
			uint8_t track_level_3:1;
			uint8_t track_level_4:1;
			uint8_t track_level_5:1;
			uint8_t track_level_6:1;
			uint8_t track_level_7:1;

			uint8_t coins_to_eject[8];
			uint32_t coin_count[8];

			uint8_t checksum;
		} configs;
	} _data;
	bool _bank;
	FRAM_MB85RC_I2C & _fram;
};

#endif
