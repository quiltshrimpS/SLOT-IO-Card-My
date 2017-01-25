#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

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

#endif
