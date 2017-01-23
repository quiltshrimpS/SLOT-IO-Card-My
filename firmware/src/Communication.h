#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#define CMD_READ_STORAGE			(0x87)
#define CMD_WRITE_STORAGE			(0x96)
#define CMD_SET_OUTPUT				(0xB4)
#define CMD_RESET_COIN_COINTER		(0xC3)
#define CMD_GET_COIN_COUNTER		(0xD2)
#define CMD_EJECT_COIN				(0xE1)
#define CMD_GET_INFO				(0xF0)

#define EVT_ERROR					(0xFF)
#define EVT_GET_INFO_RESULT			(0x0F)
#define EVT_COIN_COUNTER_RESULT		(0x2D)
#define EVT_KEY 					(0x4B)
#define EVT_WRITE_STORAGE_RESULT	(0x69)
#define EVT_READ_STORAGE_RESULT		(0x78)

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

	void dispatchCoinCounterResult(uint8_t track, uint32_t coins) {
		_messenger.sendCmdStart(EVT_COIN_COUNTER_RESULT);
		_messenger.sendCmdBinArg((uint16_t)track);
		_messenger.sendCmdBinArg(coins);
		_messenger.sendCmdEnd();
	}

	void dispatchKey(uint8_t length, uint8_t *keys) {
		_messenger.sendCmdStart(EVT_KEY);
		_messenger.sendCmdBinArg(length);
		for (uint8_t i = 0;i < length;++i)
			_messenger.sendCmdArg(keys[i]);
		_messenger.sendCmdEnd();
	}

	template < typename T >
	void dispatchError(uint8_t id, T msg) {
		_messenger.sendCmdStart(EVT_ERROR);
		_messenger.sendCmdArg(id);
		_messenger.sendCmdArg(msg);
		_messenger.sendCmdEnd();
	}

private:
	CmdMessenger & _messenger;
};

#endif
