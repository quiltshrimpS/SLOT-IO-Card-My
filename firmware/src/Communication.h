#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#define CMD_ACK						(0x00)
#define CMD_GET_INFO				(0x01)
#define CMD_GET_KEY_MASKS			(0x02)
#define CMD_GET_KEYS				(0x10)
#define CMD_SET_OUTPUT				(0x11)
#define CMD_GET_COIN_COUNTER		(0x20)
#define CMD_RESET_COIN_COINTER		(0x21)
#define CMD_TICK_AUDIT_COUNTER		(0x30)
#define CMD_EJECT_COIN				(0x40)
#define CMD_SET_TRACK_LEVEL			(0x41)
#define CMD_SET_EJECT_TIMEOUT		(0x42)
#define CMD_READ_STORAGE			(0x50)
#define CMD_WRITE_STORAGE			(0x58)
#define CMD_REBOOT					(0xFF)

#define EVT_GET_INFO_RESULT			(0x01)
#define EVT_KEY_MASKS_RESULT		(0x02)
#define EVT_KEYS_RESULT				(0x10)
#define EVT_COIN_COUNTER_RESULT		(0x20)
#define EVT_READ_STORAGE_RESULT		(0x50)
#define EVT_WRITE_STORAGE_RESULT	(0x58)
#define EVT_BOOT					(0x80)
#define EVT_DEBUG					(0xFE)
#define EVT_ERROR					(0xFF)

#define ERR_EJECT_INTERRUPTED		(0x01)
#define ERR_EJECT_TIMEOUT			(0x02)
#define ERR_NOT_A_TRACK				(0x03)
#define ERR_PROTECTED_STORAGE		(0x04)
#define ERR_TOO_LONG				(0x05)
#define ERR_NOT_A_COUNTER			(0x06)
#define ERR_OUT_OF_RANGE			(0x07)
#define ERR_UNKNOWN_COMMAND			(0xFF)

#endif
