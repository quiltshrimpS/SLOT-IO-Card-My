#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "Ports.h"
#include "Configuration.h"

#define CMD_READ_STORAGE			(0x87)
#define CMD_WRITE_STORAGE			(0x96)
#define CMD_TICK_AUDIT_COUNTER		(0x99)
#define CMD_GET_KEYS				(0xA5)
#define CMD_SET_EJECT_TIMEOUT		(0xB0)
#define CMD_SET_OUTPUT				(0xB4)
#define CMD_RESET_COIN_COINTER		(0xC3)
#define CMD_ACK						(0xC5)
#define CMD_GET_COIN_COUNTER		(0xD2)
#define CMD_GET_KEY_MASKS			(0xD5)
#define CMD_EJECT_COIN				(0xE1)
#define CMD_SET_TRACK_LEVEL			(0xEA)
#define CMD_GET_INFO				(0xF0)

#define EVT_GET_INFO_RESULT			(0x0F)
#define EVT_COIN_COUNTER_RESULT		(0x2D)
#define EVT_KEYS_RESULT				(0x4B)
#define EVT_KEY_MASKS_RESULT		(0x5D)
#define EVT_WRITE_STORAGE_RESULT	(0x69)
#define EVT_READ_STORAGE_RESULT		(0x78)
#define EVT_ERROR					(0xFF)
#define EVT_DEBUG					(0x5A)

#define ERR_EJECT_INTERRUPTED		(0x01)
#define ERR_EJECT_TIMEOUT			(0x02)
#define ERR_NOT_A_TRACK				(0x03)
#define ERR_PROTECTED_STORAGE		(0x04)
#define ERR_TOO_LONG				(0x05)
#define ERR_NOT_A_COUNTER			(0x06)
#define ERR_UNKNOWN_COMMAND			(0xFF)

class Communicator {
public:
	Communicator(CmdMessenger & messenger):
		_messenger(messenger)
	{
	}

	__attribute__((always_inline)) inline
	void dispatchGetInfoResult() {
		_messenger.sendCmdStart(EVT_GET_INFO_RESULT);
		_messenger.sendCmdArg(F("Spark"));
		_messenger.sendCmdArg(F("SLOT-IO-Card"));
		_messenger.sendCmdArg(F("v0.0.1"));
		_messenger.sendCmdBinArg<uint32_t>(20170123L);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchCoinCounterResult(uint8_t const track, uint32_t const coins) {
		_messenger.sendCmdStart(EVT_COIN_COUNTER_RESULT);
		_messenger.sendCmdBinArg<uint8_t>(track);
		_messenger.sendCmdBinArg<uint32_t>(coins);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchKeyMasksResult() {
		_messenger.sendCmdStart(EVT_KEY_MASKS_RESULT);
		_messenger.sendCmdBinArg<uint8_t>(3); // length
		_messenger.sendCmdBinArg<uint8_t>(IN_MASK_0);
		_messenger.sendCmdBinArg<uint8_t>(IN_MASK_1);
		_messenger.sendCmdBinArg<uint8_t>(IN_MASK_2);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchKeysResult(uint8_t const length, uint8_t const * const keys) {
		_messenger.sendCmdStart(EVT_KEYS_RESULT);
		_messenger.sendCmdBinArg<uint8_t>(length);
		for (uint8_t i = 0;i < length;++i)
			_messenger.sendCmdBinArg<uint8_t>(keys[i]);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchWriteStorageResult(uint16_t const address, uint8_t const length) {
		_messenger.sendCmdStart(EVT_WRITE_STORAGE_RESULT);
		_messenger.sendCmdBinArg<uint16_t>(address);
		_messenger.sendCmdBinArg<uint8_t>(length);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchReadStorageResult(uint16_t const address, uint8_t const length, uint8_t const * const buffer) {
		_messenger.sendCmdStart(EVT_READ_STORAGE_RESULT);
		_messenger.sendCmdBinArg<uint16_t>(address);
		_messenger.sendCmdBinArg<uint8_t>(length);
		for (uint8_t i = 0;i < length;++i)
			_messenger.sendCmdBinArg<uint8_t>(buffer[i]);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorEjectInterrupted(uint8_t const track, uint8_t const count) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_EJECT_INTERRUPTED);
		_messenger.sendCmdBinArg<uint8_t>(track);
		_messenger.sendCmdBinArg<uint8_t>(count);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorEjectTimeout(uint8_t const track, uint8_t const coins) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_EJECT_TIMEOUT);
		_messenger.sendCmdBinArg<uint8_t>(track);
		_messenger.sendCmdBinArg<uint8_t>(coins);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorNotATrack(uint8_t const track) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_NOT_A_TRACK);
		_messenger.sendCmdBinArg<uint8_t>(track);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorNotACounter(uint8_t const counter) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_NOT_A_COUNTER);
		_messenger.sendCmdBinArg<uint8_t>(counter);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorProtectedStorage(uint16_t const address) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_PROTECTED_STORAGE);
		_messenger.sendCmdBinArg<uint16_t>(address);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorTooLong(uint8_t length) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_TOO_LONG);
		_messenger.sendCmdBinArg<uint8_t>(MAX_BYTES_LENGTH);
		_messenger.sendCmdBinArg<uint8_t>(length);
		_messenger.sendCmdEnd();
	}

	__attribute__((always_inline)) inline
	void dispatchErrorUnknownCommand(uint8_t const command) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg<uint8_t>(ERR_UNKNOWN_COMMAND);
		_messenger.sendCmdBinArg<uint8_t>(command);
		_messenger.sendCmdEnd();
	}

private:
	CmdMessenger & _messenger;
};

#endif
