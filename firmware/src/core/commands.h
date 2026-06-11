#ifndef CORE_COMMANDS_H
#define CORE_COMMANDS_H

// Compact Command IDs
#define CMD_NOOP          0x00
#define CMD_NAVIGATE      0x01
#define CMD_EXECUTE       0x02
#define CMD_STATUS_REQ    0x03
#define CMD_STREAM_START  0x04
#define CMD_STREAM_STOP   0x05
#define CMD_MIRROR_START  0x06
#define CMD_MIRROR_STOP   0x07
#define CMD_OTA_START     0x08
#define CMD_OTA_CHUNK     0x09
#define CMD_OTA_END       0x0A
#define CMD_HANDSHAKE     0xF0

#endif // CORE_COMMANDS_H
