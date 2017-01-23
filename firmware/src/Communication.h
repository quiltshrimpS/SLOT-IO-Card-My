#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#define CMD_GET_KEYS				(0xA5)
#define CMD_READ_STORAGE			(0x87)
#define CMD_WRITE_STORAGE			(0x96)
#define CMD_SET_OUTPUT				(0xB4)
#define CMD_RESET_COIN_COINTER		(0xC3)
#define CMD_GET_COIN_COUNTER		(0xD2)
#define CMD_EJECT_COIN				(0xE1)
#define CMD_GET_INFO				(0xF0)

#define EVT_GET_INFO_RESULT			(0x0F)
#define EVT_COIN_COUNTER_RESULT		(0x2D)
#define EVT_KEYS 					(0x4B)
#define EVT_WRITE_STORAGE_RESULT	(0x69)
#define EVT_READ_STORAGE_RESULT		(0x78)
#define EVT_ERROR					(0xFF)

#define ERR_EJECT_INTERRUPTED		(0x01)
#define ERR_EJECT_TIMEOUT			(0x02)
#define ERR_NOT_A_TRACK				(0x03)
#define ERR_PROTECTED_STORAGE		(0x04)
#define ERR_UNKNOWN_COMMAND			(0xFF)

class Communicator {
public:
	Communicator(CmdMessenger & messenger):
		_messenger(messenger)
	{
	}

	void dispatchGetInfoResult() {
		_messenger.sendCmdStart(EVT_GET_INFO_RESULT);
		_messenger.sendCmdArg(F("Spark"));
		_messenger.sendCmdArg(F("SLOT-IO-Card"));
		_messenger.sendCmdArg(F("v0.0.1"));
		_messenger.sendCmdBinArg(20170123L);
		_messenger.sendCmdEnd();
	}

	void dispatchCoinCounterResult(uint8_t const track, uint32_t const coins) {
		_messenger.sendCmdStart(EVT_COIN_COUNTER_RESULT);
		_messenger.sendCmdBinArg((uint16_t)track);
		_messenger.sendCmdBinArg(coins);
		_messenger.sendCmdEnd();
	}

	void dispatchKey(uint8_t const length, uint8_t const * const keys) {
		_messenger.sendCmdStart(EVT_KEYS);
		_messenger.sendCmdBinArg(length);
		for (uint8_t i = 0;i < length;++i)
			_messenger.sendCmdBinArg(keys[i]);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorEjectInterrupted(uint8_t const track, uint8_t const count) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg(ERR_EJECT_INTERRUPTED);
		_messenger.sendCmdBinArg(track);
		_messenger.sendCmdBinArg(count);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorEjectTimeout(uint8_t const track) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg(ERR_EJECT_TIMEOUT);
		_messenger.sendCmdBinArg(track);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorNotATrack(uint8_t const track) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg(ERR_NOT_A_TRACK);
		_messenger.sendCmdBinArg(track);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorProtectedStorage(uint16_t const address) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg(ERR_PROTECTED_STORAGE);
		_messenger.sendCmdBinArg(address);
		_messenger.sendCmdEnd();
	}

	void dispatchErrorUnknownCommand(uint8_t const command) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdBinArg(ERR_UNKNOWN_COMMAND);
		_messenger.sendCmdBinArg(command);
		_messenger.sendCmdEnd();
	}

private:
	CmdMessenger & _messenger;
};

#endif
