#ifndef MSG_H
#define MSG_H

#include <stdint.h>

#define MSG_AUTH_REQ 0x01
#define MSG_AUTH_RESP 0x02
#define MSG_CMD 0x03
#define MSG_CMD_ACK 0x04
#define MSG_CMD_RESP 0x05
#define MSG_ERROR 0xFF

#pragma pack(push,1)

struct PackHeader{
    uint8_t type;
    uint32_t length;
};
#pragma pack(pop)

#endif