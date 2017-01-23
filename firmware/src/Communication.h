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

#endif
