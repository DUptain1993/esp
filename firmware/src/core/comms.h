#pragma once
#include <Arduino.h>
#include "protocol.h"
#include "encryption.h"
#include "compression.h"

// Comms payload flag bits (first byte of the protocol PAYLOAD).
// Wire payload layout: [FLAGS][MSG_LEN][PAD_LEN][BODY...]
#define COMMS_FLAG_RLE   0x01  // body is RLE compressed
#define COMMS_FLAG_DELTA 0x02  // body is delta encoded (applied before RLE)
#define COMMS_FLAG_ENC   0x04  // body is encrypted

// Handler invoked for each decoded, plaintext application message.
typedef void (*comms_msg_handler_t)(uint8_t channel, uint8_t device_id,
                                    const uint8_t *msg, uint8_t len);

// Initialise comms over the given UART (defaults to USB Serial @ 115200).
void comms_init(HardwareSerial *port = &Serial, uint32_t baud = 115200);

// Register the application message dispatcher.
void comms_set_handler(comms_msg_handler_t h);

// Enable/disable obfuscation (random padding, jitter, noise packets).
void comms_set_obfuscation(bool on);

// Seed the rolling cipher (call after a handshake). Both TX and RX streams.
void comms_set_nonce(uint32_t nonce);

// Non-blocking pump: drains the UART, parses frames, dispatches messages,
// and periodically emits noise packets. Call from the comms task.
void comms_task(void);

// Send an application message. Compression auto-enables for large payloads.
// Returns true if the frame was queued to the transport.
bool comms_send(uint8_t channel, uint8_t device_id,
                const uint8_t *msg, uint8_t len,
                bool encrypt = true, bool compress = true);

// Convenience: single-opcode control message.
bool comms_send_cmd(uint8_t channel, uint8_t device_id, uint8_t cmd);

// Diagnostics.
uint32_t comms_rx_good(void);
uint32_t comms_rx_bad(void);
uint32_t comms_last_pong(void);
