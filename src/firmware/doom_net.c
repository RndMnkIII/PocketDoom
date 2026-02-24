/*
 * Doom network stubs for Analogue Pocket (PocketDoom)
 */

#include <stdlib.h>

#include "doomstat.h"
#include "d_net.h"
#include "i_net.h"

void I_InitNetwork(void)
{
    doomcom = calloc(1, sizeof(*doomcom));
    doomcom->ticdup = 1;

    netgame = false;

    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
}

void I_NetCmd(void)
{
}
