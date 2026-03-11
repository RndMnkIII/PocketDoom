/*
 * Doom network driver for Analogue Pocket link cable (PocketDoom)
 *
 * Uses link_mmio hardware peripheral for 2-player multiplayer over
 * the GBA link port.
 *
 * Protocol: simple framed unreliable packets with CRC16. Doom's own
 * d_net.c handles retransmission (NCMD_RETRANSMIT).
 *
 * Handshake: host sends HELLO, join responds with HELLO_ACK.
 * Then D_ArbitrateNetStart handles game settings exchange.
 *
 * The menu is handled in m_menu.c — this file provides the low-level
 * link API (I_LinkHasHardware, I_LinkConnect, I_LinkSetupNet) and
 * the Doom network interface (I_InitNetwork, I_NetCmd).
 */

#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_net.h"
#include "i_net.h"
#include "i_system.h"

/* ============================================
 * Controller registers (direct read for cancel check)
 * ============================================ */

#define SYS_BASE        0x40000000
#define SYS_CONT1_KEY   (*(volatile uint32_t *)(SYS_BASE + 0x50))

#define PAD_FACE_B      (1 << 5)

/* ============================================
 * Link MMIO registers (0x4D000000)
 * ============================================ */

#define LINK_BASE       0x4D000000
#define LINK_REG_ID     (*(volatile uint32_t *)(LINK_BASE + 0x00))
#define LINK_REG_STATUS (*(volatile uint32_t *)(LINK_BASE + 0x08))
#define LINK_REG_CTRL   (*(volatile uint32_t *)(LINK_BASE + 0x0C))
#define LINK_REG_TX     (*(volatile uint32_t *)(LINK_BASE + 0x10))
#define LINK_REG_RX     (*(volatile uint32_t *)(LINK_BASE + 0x14))
#define LINK_REG_TXSP   (*(volatile uint32_t *)(LINK_BASE + 0x18))
#define LINK_REG_RXCNT  (*(volatile uint32_t *)(LINK_BASE + 0x1C))

#define LINK_ID_MAGIC   0x4C4E4B31  /* "LNK1" */

/* CTRL bits */
#define LCTRL_ENABLE    (1 << 0)
#define LCTRL_RESET     (1 << 1)
#define LCTRL_CLEARERR  (1 << 2)
#define LCTRL_FLUSH_RX  (1 << 3)
#define LCTRL_FLUSH_TX  (1 << 4)
#define LCTRL_MASTER    (1 << 5)
#define LCTRL_POLL      (1 << 6)

/* ============================================
 * Frame protocol
 * ============================================ */

#define FRAME_MAGIC     0x444D4E54  /* "DMNT" */

#define PKT_HELLO       1
#define PKT_HELLO_ACK   2
#define PKT_GAMEDATA    3

/* Timing (I_GetTime ticks = 35 Hz) */
#define CONNECT_TIMEOUT     245     /* 7 seconds */
#define HELLO_INTERVAL      4       /* ~115ms */

/* Max payload: doomdata_t ~ 104 bytes, round up */
#define MAX_PAYLOAD         128

/* ============================================
 * CRC16-CCITT
 * ============================================ */

static uint16_t crc16_byte(uint16_t crc, uint8_t b)
{
    crc ^= (uint16_t)b << 8;
    for (int i = 0; i < 8; i++)
        crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    return crc;
}

static uint16_t crc16_hdr(int type, int len)
{
    uint16_t crc = 0xFFFF;
    crc = crc16_byte(crc, type);
    crc = crc16_byte(crc, len & 0xFF);
    crc = crc16_byte(crc, (len >> 8) & 0xFF);
    return crc;
}

/* ============================================
 * Driver state
 * ============================================ */

static int link_connected;

/* RX frame parser */
enum { RX_MAGIC, RX_HEADER, RX_CRC, RX_PAYLOAD };
static int rx_state;
static int rx_type, rx_len;
static uint16_t rx_crc;
static int rx_words_need, rx_words_got;
static uint8_t rx_buf[MAX_PAYLOAD];

/* Received game packet (single-buffered) */
static int rx_pkt_ready;
static uint8_t rx_pkt_data[MAX_PAYLOAD];
static int rx_pkt_len;

/* ============================================
 * Low-level link I/O
 * ============================================ */

static void link_hw_reset(void)
{
    LINK_REG_CTRL = LCTRL_RESET;
    for (volatile int i = 0; i < 100; i++);
    LINK_REG_CTRL = LCTRL_CLEARERR | LCTRL_FLUSH_RX | LCTRL_FLUSH_TX;
    for (volatile int i = 0; i < 100; i++);
    rx_state = RX_MAGIC;
    link_connected = 0;
    rx_pkt_ready = 0;
}

static void link_send_frame(int type, const void *data, int len)
{
    int nwords = 3 + (len + 3) / 4;

    for (int t = 0; t < 50000; t++)
        if (LINK_REG_TXSP >= (uint32_t)nwords)
            break;

    uint16_t crc = crc16_hdr(type, len);
    const uint8_t *p = data;
    for (int i = 0; i < len; i++)
        crc = crc16_byte(crc, p[i]);

    LINK_REG_TX = FRAME_MAGIC;
    LINK_REG_TX = ((uint32_t)type << 16) | (uint32_t)(len & 0xFFFF);
    LINK_REG_TX = (uint32_t)crc;

    for (int i = 0; i < len; i += 4) {
        uint32_t w = 0;
        for (int j = 0; j < 4 && (i + j) < len; j++)
            w |= (uint32_t)p[i + j] << (j * 8);
        LINK_REG_TX = w;
    }
}

static int link_rx_process(uint32_t word)
{
    switch (rx_state) {
    case RX_MAGIC:
        if (word == FRAME_MAGIC)
            rx_state = RX_HEADER;
        return 0;

    case RX_HEADER:
        rx_type = (word >> 16) & 0xFFFF;
        rx_len = word & 0xFFFF;
        if (rx_len > MAX_PAYLOAD) {
            rx_state = RX_MAGIC;
            return 0;
        }
        rx_state = RX_CRC;
        return 0;

    case RX_CRC:
        rx_crc = word & 0xFFFF;
        if (rx_len == 0) {
            rx_state = RX_MAGIC;
            uint16_t chk = crc16_hdr(rx_type, 0);
            return (chk == rx_crc) ? rx_type : 0;
        }
        rx_words_need = (rx_len + 3) / 4;
        rx_words_got = 0;
        rx_state = RX_PAYLOAD;
        return 0;

    case RX_PAYLOAD: {
        int base = rx_words_got * 4;
        for (int j = 0; j < 4 && (base + j) < rx_len; j++)
            rx_buf[base + j] = (word >> (j * 8)) & 0xFF;
        rx_words_got++;
        if (rx_words_got < rx_words_need)
            return 0;
        rx_state = RX_MAGIC;
        uint16_t chk = crc16_hdr(rx_type, rx_len);
        for (int i = 0; i < rx_len; i++)
            chk = crc16_byte(chk, rx_buf[i]);
        return (chk == rx_crc) ? rx_type : 0;
    }
    }

    rx_state = RX_MAGIC;
    return 0;
}

static void link_pump_rx(void)
{
    int budget = 256;
    while (LINK_REG_RXCNT > 0 && --budget > 0) {
        uint32_t w = LINK_REG_RX;
        int ptype = link_rx_process(w);

        if (ptype == PKT_GAMEDATA && link_connected) {
            memcpy(rx_pkt_data, rx_buf, rx_len);
            rx_pkt_len = rx_len;
            rx_pkt_ready = 1;
        }
    }
}

/* ============================================
 * Exported link cable API (called from m_menu.c)
 * ============================================ */

int I_LinkHasHardware(void)
{
    return (LINK_REG_ID == LINK_ID_MAGIC);
}

int I_LinkConnect(int is_host)
{
    link_hw_reset();

    if (is_host)
        LINK_REG_CTRL = LCTRL_ENABLE | LCTRL_MASTER | LCTRL_POLL;
    else
        LINK_REG_CTRL = LCTRL_ENABLE;

    rx_state = RX_MAGIC;
    int last_hello = 0;
    int connected = 0;
    int t0 = I_GetTime();

    while (I_GetTime() - t0 < CONNECT_TIMEOUT) {
        if (SYS_CONT1_KEY & PAD_FACE_B)
            goto fail;

        int now = I_GetTime();

        if (is_host) {
            if (now - last_hello >= HELLO_INTERVAL) {
                link_send_frame(PKT_HELLO, NULL, 0);
                last_hello = now;
            }
            while (LINK_REG_RXCNT > 0) {
                uint32_t w = LINK_REG_RX;
                int ptype = link_rx_process(w);
                if (ptype == PKT_HELLO_ACK) {
                    connected = 1;
                    break;
                }
            }
        } else {
            while (LINK_REG_RXCNT > 0) {
                uint32_t w = LINK_REG_RX;
                int ptype = link_rx_process(w);
                if (ptype == PKT_HELLO) {
                    link_send_frame(PKT_HELLO_ACK, NULL, 0);
                    link_send_frame(PKT_HELLO_ACK, NULL, 0);
                    link_send_frame(PKT_HELLO_ACK, NULL, 0);
                    connected = 1;
                    break;
                }
            }
        }

        if (connected)
            break;
    }

    if (!connected)
        goto fail;

    link_connected = 1;
    return 1;

fail:
    link_hw_reset();
    return -1;
}

void I_LinkSetupNet(int is_host, int dm)
{
    netgame = true;
    deathmatch = dm;
    nomonsters = dm;
    doomcom->numplayers = 2;
    doomcom->numnodes = 2;
    doomcom->consoleplayer = is_host ? 0 : 1;
    doomcom->deathmatch = dm;
}

/* ============================================
 * Doom network interface
 * ============================================ */

void I_InitNetwork(void)
{
    doomcom = calloc(1, sizeof(*doomcom));
    doomcom->ticdup = 1;
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
    netgame = false;
}

void I_NetCmd(void)
{
    if (!link_connected)
        return;

    link_pump_rx();

    if (doomcom->command == CMD_SEND) {
        if (doomcom->remotenode > 0) {
            link_send_frame(PKT_GAMEDATA,
                            &doomcom->data,
                            doomcom->datalength);
        }
    } else if (doomcom->command == CMD_GET) {
        if (rx_pkt_ready) {
            memcpy(&doomcom->data, rx_pkt_data, rx_pkt_len);
            doomcom->remotenode = 1;
            doomcom->datalength = rx_pkt_len;
            rx_pkt_ready = 0;
        } else {
            doomcom->remotenode = -1;
        }
    }
}
