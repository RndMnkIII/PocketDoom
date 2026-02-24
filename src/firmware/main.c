/*
 * PocketDoom Bootloader
 * Runs from BRAM, loads doom.bin + WAD via deferload to SDRAM, jumps to Doom
 */

#include "dataslot.h"
#include "terminal.h"

/* Debug variables (read by misaligned trap handler) */
volatile unsigned int pd_dbg_stage = 0;
volatile unsigned int pd_dbg_info = 0;

/* System registers */
#define SYS_STATUS      (*(volatile unsigned int *)0x40000000)
#define SYS_CYCLE_LO    (*(volatile unsigned int *)0x40000004)
#define SYS_DISPLAY_MODE (*(volatile unsigned int *)0x4000000C)

/* Data slot IDs (match data.json) */
#define WAD_SLOT_ID     0
#define DOOM_SLOT_ID    1

/* External symbols from linker */
extern char _doom_bss_start[], _doom_bss_end[];
extern char _runtime_stack_top[];
extern char _doom_load_addr[];  /* SDRAM address where doom.bin is loaded */
extern char _doom_copy_size[];  /* Size of .text + .data */
/* Doom entry point (in doom_pocket.c, linked in SDRAM) */
extern void doom_main(void);
extern void switch_to_runtime_stack_and_call(void (*entry)(void), void *stack_top);

/* WAD preload address in SDRAM */
#define WAD_SDRAM_ADDR  0x10C00000
/* WAD maximum size (bridge offset 0x00C00000 to end of 64MB SDRAM) */
#define WAD_MAX_SIZE    (52 * 1024 * 1024)

/* Clear BSS section */
__attribute__((section(".text.boot")))
static void clear_doom_bss(void) {
    unsigned int *p = (unsigned int *)_doom_bss_start;
    unsigned int *end = (unsigned int *)_doom_bss_end;

    while (p < end)
        *p++ = 0;
}

/* Flush I-cache after loading doom.bin to SDRAM (ensures stale cache lines
 * from previous runs don't cause code corruption). */
__attribute__((section(".text.boot")))
static void flush_icache(void) {
    __asm__ volatile("fence");
    __asm__ volatile(".word 0x0000100f");  /* fence.i */
}

/* Load from a data slot into SDRAM via deferload, chunked.
 * slot_offset: byte offset within the slot to start reading from.
 * dest: CPU SDRAM address to load into.
 * total: number of bytes to load. */
__attribute__((section(".text.boot")))
static int load_slot(uint32_t slot_id, uint32_t slot_offset, void *dest, uint32_t total) {
    uint32_t done = 0;

    while (done < total) {
        uint32_t chunk = total - done;
        if (chunk > DMA_CHUNK_SIZE)
            chunk = DMA_CHUNK_SIZE;

        int rc = dataslot_read(slot_id, slot_offset + done,
                               (void *)((uint32_t)dest + done), chunk);
        if (rc < 0)
            return rc;

        done += chunk;
    }

    return 0;
}

__attribute__((section(".text.boot")))
int main(void) {
    term_init();
    term_printf("PocketDoom Boot\n\n");

    /* Wait for APF bridge allcomplete before issuing dataslot commands */
    term_printf("Waiting for bridge...\n");
    unsigned int start_wait = SYS_CYCLE_LO;
    while (!(SYS_STATUS & (1 << 1))) {
        if ((SYS_CYCLE_LO - start_wait) > 500000000)  /* 5s timeout */
            break;
    }

    /* Load doom.bin from data slot directly into SDRAM (executes in place) */
    uint32_t doom_size = (uint32_t)_doom_copy_size;
    term_printf("Loading doom.bin (%d bytes)...\n", doom_size);
    int rc = load_slot(DOOM_SLOT_ID, 0, _doom_load_addr, doom_size);
    if (rc < 0) {
        term_printf("FAILED to load doom.bin (rc=%d)\n", rc);
        while (1) {}
    }
    term_printf("doom.bin loaded OK\n");

    /* Load WAD header first to determine size, then load the rest */
    term_printf("Loading WAD header...\n");
    rc = load_slot(WAD_SLOT_ID, 0, (void *)WAD_SDRAM_ADDR, DMA_CHUNK_SIZE);
    if (rc < 0) {
        term_printf("FAILED to load WAD header (rc=%d)\n", rc);
        while (1) {}
    }

    /* Parse WAD header: char[4] magic, int32 numlumps, int32 diroffset */
    volatile unsigned int *wad = (volatile unsigned int *)SDRAM_UNCACHED(WAD_SDRAM_ADDR);
    unsigned int magic = wad[0];
    unsigned int numlumps = wad[1];
    unsigned int diroffset = wad[2];
    /* Total WAD size = directory offset + numlumps * 16 (each entry is 16 bytes) */
    unsigned int wad_size = diroffset + numlumps * 16;

    term_printf("WAD: %s numlumps=%d dir@%x size=%d\n",
                (magic == 0x44415749) ? "IWAD" :
                (magic == 0x44415750) ? "PWAD" : "BAD!",
                numlumps, diroffset, wad_size);

    if (magic != 0x44415749 && magic != 0x44415750) {
        term_printf("FAILED: not a valid WAD file\n");
        while (1) {}
    }

    /* Load remaining WAD data (first chunk already loaded) */
    if (wad_size > DMA_CHUNK_SIZE) {
        uint32_t remaining = wad_size - DMA_CHUNK_SIZE;
        term_printf("Loading WAD body (%d bytes)...\n", remaining);
        rc = load_slot(WAD_SLOT_ID, DMA_CHUNK_SIZE,
                       (void *)(WAD_SDRAM_ADDR + DMA_CHUNK_SIZE), remaining);
        if (rc < 0) {
            term_printf("FAILED to load WAD (rc=%d)\n", rc);
            while (1) {}
        }
    }
    term_printf("WAD loaded OK\n");

    /* Flush I-cache (doom.bin loaded via DMA, may have stale cache lines) */
    flush_icache();

    /* Clear BSS */
    clear_doom_bss();

    /* Jump to Doom */
    term_printf("Starting Doom...\n");
    switch_to_runtime_stack_and_call(doom_main, _runtime_stack_top);

    while (1) {}
    return 0;
}
