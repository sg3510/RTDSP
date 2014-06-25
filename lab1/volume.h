/*
 *  Copyright 2003 by Texas Instruments Incorporated.
 *  All rights reserved. Property of Texas Instruments Incorporated.
 *  Restricted rights to use, duplicate or disclose this code are
 *  granted through contract.
 *  
 */
/* "@(#) DSP/BIOS 4.90.270 01-08-04 (bios,dsk6713-c04)" */
/*
 *  ======== volume.h ========
 *
 */

#ifndef __VOLUME_H
#define __VOLUME_H

#ifndef TRUE
#define TRUE 1
#endif

#define BUFSIZE 0x64

#define FRAMESPERBUFFER 10

#define MINGAIN 1
#define MAXGAIN 10

#define MINCONTROL 0
#define MAXCONTROL 19

#define BASELOAD 1

struct PARMS {
	int Beta;
	int EchoPower;
	int ErrorPower;
	int Ratio;
	struct PARMS *Link;
};

#endif /* __VOLUME_H */
