// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      System specific network interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_NET__
#define __I_NET__


#ifdef __GNUG__
#pragma interface
#endif



// Called by D_DoomMain.

void I_InitNetwork (void);
void I_NetCmd (void);

// Link cable API (Analogue Pocket)
int  I_LinkHasHardware(void);           // 1 if LNK1 present
int  I_LinkConnect(int is_host);        // 1=success, -1=fail/cancel
void I_LinkSetupNet(int is_host, int dm); // configure doomcom for 2P


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
