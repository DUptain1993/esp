#include "comms.h"
#include "commands.h"
#include <esp_system.h>

static HardwareSerial *s_port = &Serial;
static ProtocolParser  s_parser;
static comms_msg_handler_t s_handler = nullptr;

// Independent cipher streams for each direction (kept in lock-step with peer).
static Crypto s_tx_crypto;
static Crypto s_rx_crypto;

static bool     s_obf = true;
static uint32_t s_last_noise = 0;
static uint32_t s_noise_interval = 4000; // ms between noise packets

static uint32_t s_last_pong = 0;

// Scratch buffers (single-threaded comms task usage).
static uint8_t s_frame[PROTO_MAX_PAYLOAD + 16];
static uint8_t s_work[PROTO_MAX_PAYLOAD + 16];
static uint8_t s_tmp[PROTO_MAX_PAYLOAD + 16];

// ---------------------------------------------------------------------------

static inline uint32_t rnd(uint32_t lo, uint32_t hi)
{
    if (hi <= lo) return lo;
    return lo + (esp_random() % (hi - lo + 1));
}

static void on_packet(const ProtoPacket *pkt)
{
    // Decode wire payload: [FLAGS][MSG_LEN][PAD_LEN][BODY...]
    if (pkt->len < 3) return;
    uint8_t flags   = pkt->payload[0];
    uint8_t msg_len = pkt->payload[1];
    uint8_t pad_len = pkt->payload[2];
    const uint8_t *body = &pkt->payload[3];
    int body_len = (int)pkt->len - 3;
    if (body_len < 0) return;

    // Copy body for in-place transforms.
    if (body_len > (int)sizeof(s_work)) return;
    memcpy(s_work, body, body_len);

    // 1) Decrypt.
    if (flags & COMMS_FLAG_ENC) {
        s_rx_crypto.decrypt(s_work, body_len);
    }

    // 2) Decompress (RLE then delta, mirroring the encode order).
    uint8_t *plain = s_work;
    int plain_len = body_len;
    if (flags & COMMS_FLAG_RLE) {
        int n = (int)compression::rle_decode(s_work, body_len, s_tmp, sizeof(s_tmp));
        if (n <= 0) return;
        if (flags & COMMS_FLAG_DELTA) {
            int m = (int)compression::delta_decode(s_tmp, n, s_work, sizeof(s_work));
            if (m <= 0) return;
            plain = s_work;
            plain_len = m;
        } else {
            plain = s_tmp;
            plain_len = n;
        }
    } else if (flags & COMMS_FLAG_DELTA) {
        int m = (int)compression::delta_decode(s_work, body_len, s_tmp, sizeof(s_tmp));
        if (m <= 0) return;
        plain = s_tmp;
        plain_len = m;
    }

    // Validate recovered length and strip padding.
    int expected = (int)msg_len + (int)pad_len;
    if (plain_len < expected || msg_len > plain_len) return;

    // Drop obfuscation noise packets.
    if (pkt->channel == CH_CONTROL && msg_len >= 1 && plain[0] == CMD_NOISE) {
        return;
    }

    if (s_handler) {
        if (msg_len >= 1 && pkt->channel == CH_CONTROL && plain[0] == CMD_PONG) {
            s_last_pong = millis();
        }
        s_handler(pkt->channel, pkt->device_id, plain, msg_len);
    }
}

// ---------------------------------------------------------------------------

void comms_init(HardwareSerial *port, uint32_t baud)
{
    s_port = port ? port : &Serial;
    s_port->begin(baud);
    s_parser.on_packet(on_packet);
    s_tx_crypto.begin(CRYPTO_SHARED_SECRET, 0);
    s_rx_crypto.begin(CRYPTO_SHARED_SECRET, 0);
    s_last_noise = millis();
}

void comms_set_handler(comms_msg_handler_t h) { s_handler = h; }
void comms_set_obfuscation(bool on) { s_obf = on; }

void comms_set_nonce(uint32_t nonce)
{
    s_tx_crypto.set_nonce(nonce);
    s_rx_crypto.set_nonce(nonce);
}

uint32_t comms_rx_good(void) { return s_parser.good_packets(); }
uint32_t comms_rx_bad(void)  { return s_parser.bad_packets(); }
uint32_t comms_last_pong(void) { return s_last_pong; }

// ---------------------------------------------------------------------------

static bool transmit(uint8_t channel, uint8_t device_id,
                     const uint8_t *msg, uint8_t len,
                     bool encrypt, bool compress, bool jitter)
{
    if (len > 0 && !msg) return false;

    // Build [msg | padding] into work buffer.
    uint8_t pad_len = s_obf ? (uint8_t)rnd(1, 4) : 0;
    int total = (int)len + (int)pad_len;
    if (total > (int)sizeof(s_work)) return false;

    memcpy(s_work, msg, len);
    for (uint8_t i = 0; i < pad_len; i++) s_work[len + i] = (uint8_t)esp_random();

    uint8_t flags = 0;
    uint8_t *body = s_work;
    int body_len = total;

    // Compression: delta -> RLE, keep only if it shrinks the payload.
    if (compress && compression::should_compress(total)) {
        int d = (int)compression::delta_encode(s_work, total, s_tmp, sizeof(s_tmp));
        if (d > 0) {
            uint8_t rle[PROTO_MAX_PAYLOAD + 16];
            int r = (int)compression::rle_encode(s_tmp, d, rle, sizeof(rle));
            if (r > 0 && r < total) {
                memcpy(s_work, rle, r);
                body_len = r;
                flags |= (COMMS_FLAG_DELTA | COMMS_FLAG_RLE);
            }
        }
    }

    // Encryption.
    if (encrypt) {
        s_tx_crypto.encrypt(body, body_len);
        flags |= COMMS_FLAG_ENC;
    }

    // Assemble wire payload: [FLAGS][MSG_LEN][PAD_LEN][BODY...]
    int payload_len = 3 + body_len;
    if (payload_len > PROTO_MAX_PAYLOAD) return false;
    uint8_t payload[PROTO_MAX_PAYLOAD];
    payload[0] = flags;
    payload[1] = len;
    payload[2] = pad_len;
    memcpy(&payload[3], body, body_len);

    size_t n = proto_build(channel, device_id, payload, (uint8_t)payload_len,
                           s_frame, sizeof(s_frame));
    if (n == 0) return false;

    // Packet jitter (only on background/obfuscated traffic, never urgent UI).
    if (jitter && s_obf) {
        vTaskDelay(pdMS_TO_TICKS(rnd(15, 90)));
    }

    s_port->write(s_frame, n);
    return true;
}

bool comms_send(uint8_t channel, uint8_t device_id,
                const uint8_t *msg, uint8_t len,
                bool encrypt, bool compress)
{
    return transmit(channel, device_id, msg, len, encrypt, compress, false);
}

bool comms_send_cmd(uint8_t channel, uint8_t device_id, uint8_t cmd)
{
    uint8_t m = cmd;
    return comms_send(channel, device_id, &m, 1, true, false);
}

// ---------------------------------------------------------------------------

void comms_task(void)
{
    // Drain RX without blocking.
    while (s_port->available() > 0) {
        int b = s_port->read();
        if (b < 0) break;
        s_parser.feed((uint8_t)b);
    }

    // Emit a noise packet on the control channel periodically.
    if (s_obf) {
        uint32_t now = millis();
        if (now - s_last_noise >= s_noise_interval) {
            s_last_noise = now;
            uint8_t noise[5];
            noise[0] = CMD_NOISE;
            uint8_t nl = (uint8_t)rnd(1, 4);
            for (uint8_t i = 0; i < nl; i++) noise[1 + i] = (uint8_t)esp_random();
            transmit(CH_CONTROL, DEVICE_BROADCAST, noise, 1 + nl, true, false, true);
            s_noise_interval = rnd(3000, 7000);
        }
    }
}
