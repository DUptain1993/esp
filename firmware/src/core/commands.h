#pragma once
#include <stdint.h>

// ============================================================================
//  Binary protocol channels
//  Packet: [0xAA][CHANNEL][DEVICE_ID][LEN][PAYLOAD...][CHK][0x55]
// ============================================================================

// Frame delimiters
#define PROTO_SOF 0xAA   // Start Of Frame
#define PROTO_EOF 0x55   // End Of Frame

// Channels
enum ProtoChannel : uint8_t {
    CH_CONTROL = 0x01,
    CH_STATUS  = 0x02,
    CH_STREAM  = 0x03,
    CH_OTA     = 0x04,
    CH_ROUTING = 0x05,
};

// Device addressing
#define DEVICE_BROADCAST 0xFF   // Broadcast to all devices
#define DEVICE_SELF      0x01   // Default local device id

// Maximum payload that fits in a single packet (LEN is one byte)
#define PROTO_MAX_PAYLOAD 255

// ----------------------------------------------------------------------------
//  Command opcodes (first payload byte after the comms flag header)
// ----------------------------------------------------------------------------

// CH_CONTROL commands
enum CtrlCommand : uint8_t {
    CMD_NOISE        = 0x00,  // obfuscation noise packet (ignored)
    CMD_PING         = 0x10,
    CMD_PONG         = 0x11,
    CMD_HANDSHAKE    = 0x12,  // challenge / nonce exchange
    CMD_SET_MODE     = 0x20,  // terminal vs graphical mirror
    CMD_QUICK_ACTION = 0x21,
    CMD_EXECUTE      = 0x22,
    CMD_PAGE_NEXT    = 0x23,
    CMD_PAGE_PREV    = 0x24,
    CMD_INPUT        = 0x25,  // Typed keystrokes ESP32 -> Pi
};

// CH_STATUS commands
enum StatusCommand : uint8_t {
    ST_HEARTBEAT = 0x30,
    ST_DEVICE_UP = 0x31,
    ST_DEVICE_DN = 0x32,
    ST_INFO      = 0x33,
};

// CH_STREAM commands
enum StreamCommand : uint8_t {
    STR_DATA   = 0x40,  // raw / ANSI terminal bytes
    STR_CLEAR  = 0x41,
    STR_MIRROR = 0x42,  // mirror draw-command / framebuffer-diff ops
};

// Mirror graphical draw op-codes (payload of a STR_MIRROR message)
enum MirrorOp : uint8_t {
    MOP_FILL  = 0x01,  // [colorLo][colorHi]            : clear canvas
    MOP_PIXEL = 0x02,  // [x:2][y:2][color:2]           : set one pixel
    MOP_RECT  = 0x03,  // [x:2][y:2][w:2][h:2][color:2] : filled rect
    MOP_HLINE = 0x04,  // [x:2][y:2][len:2][color:2]    : horizontal line
};

// CH_OTA commands
enum OtaCommand : uint8_t {
    OTA_BEGIN  = 0x50,  // payload: 4-byte total size (LE)
    OTA_CHUNK  = 0x51,  // payload: 2-byte seq (LE) + data
    OTA_END    = 0x52,  // payload: 4-byte crc/size (LE)
    OTA_ABORT  = 0x53,
    OTA_PROGRESS = 0x54, // payload: 1-byte pct
    OTA_DONE     = 0x55,
    OTA_FAIL     = 0x56,
};

// CH_ROUTING commands
enum RouteCommand : uint8_t {
    RT_SELECT     = 0x60,  // payload: target device id
    RT_LIST       = 0x61,
    RT_BROADCAST  = 0x62,
};

// ----------------------------------------------------------------------------
//  Mirror render modes
// ----------------------------------------------------------------------------
enum MirrorMode : uint8_t {
    MIRROR_TERMINAL  = 0x00,
    MIRROR_GRAPHICAL = 0x01,
};
