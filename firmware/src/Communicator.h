#ifndef __COMMUNICATOR_H__
#define __COMMUNICATOR_H__

#include <CmdMessenger.h>

#include "Ports.h"
#include "Communication.h"
#include "Configuration.h"

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
