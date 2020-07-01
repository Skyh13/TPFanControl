
// --------------------------------------------------------------
//
//  Thinkpad Fan Control
//
// --------------------------------------------------------------
//
// The original TPFanControl code is in public domain. See this fork for public domain code:
// https://github.com/cjbdev/TPFanControl
// All my own patches are licensed under GNU GPL v3 license. See notice below and LICENSE file for details.
//
// Copyright (c) Stanislav Vinokurov, 2020
//
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see < https://www.gnu.org/licenses/>.// 
//
// --------------------------------------------------------------

#include "_prec.h"
#include "fancontrol.h"
#include "TVicPort.h"


// Registers of the embedded controller
#define EC_DATAPORT	0x1600	// EC data io-port 0x62
#define EC_CTRLPORT	0x1604	// EC control io-port 0x66


// Embedded controller status register bits
#define EC_STAT_OBF	 0x01 // Output buffer full 
#define EC_STAT_IBF	 0x02 // Input buffer full 
#define EC_STAT_CMD	 0x08 // Last write was a command write (0=data) 


// Embedded controller commands
// (write to EC_CTRLPORT to initiate read/write operation)
#define EC_CTRLPORT_READ	 (char)0x80	
#define EC_CTRLPORT_WRITE	 (char)0x81
#define EC_CTRLPORT_QUERY	 (char)0x84


int verbosity = 0;	// verbosity for the logbuf (0= nothing)
char lasterrorstring[256] = "",
logbuf[8192] = "";


//-------------------------------------------------------------------------
// read a byte from the embedded controller (EC) via port io 
//-------------------------------------------------------------------------
int FANCONTROL::ReadByteFromEC(int offset, char* pdata)
{
	char data = -1;
	int iOK = false;
	int iTimeout = 100;
	int iTimeoutBuf = 1000;
	int	iTime = 0;
	int iTick = 10;
	int numSetupTries = 5;

	while (numSetupTries > 0) {

		for (iTime = 0; iTime < iTimeoutBuf; iTime += iTick) {	// wait for full buffers to clear
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;			// or timeout iTimeoutBuf = 1000
			if (!(data & (EC_STAT_IBF | EC_STAT_OBF))) {
				iOK = true;
				break;
			}
			::Sleep(iTick);
		}

		if (!iOK) return 0;
		iOK = false;

		WritePort(EC_CTRLPORT, EC_CTRLPORT_READ);			// tell 'em we want to "READ"

		for (iTime = 0; iTime < iTimeout; iTime += iTick) {	// wait for IBF and OBF to clear
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;
			if (!(data & (EC_STAT_IBF | EC_STAT_OBF))) {
				iOK = true;
				break;
			}
			::Sleep(iTick);
		} // check again after a moment

		if (!iOK) return 0;
		iOK = false;

		WritePort(EC_DATAPORT, offset);						// tell 'em where we want to read from

		for (iTime = 0; iTime < iTimeout; iTime += iTick) { // wait for OBF 
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;
			if ((data & EC_STAT_OBF)) {
				iOK = true;
				numSetupTries = 1;
				break;
			}
			::Sleep(iTick); // check again after a moment
		}

		// decrement counter. If greater than 0 afterwards,
		// start the read process over again
		numSetupTries -= 1;
	}
	if (!iOK) return 0;

	*pdata = ReadPort(EC_DATAPORT);

	if (verbosity > 0) sprintf(logbuf + strlen(logbuf), "readec: offset= %x, data= %02x\n", offset, *pdata);

	return 1;
}


//-------------------------------------------------------------------------
// write a byte to the embedded controller (EC) via port io
//-------------------------------------------------------------------------
int FANCONTROL::WriteByteToEC(int offset, char NewData)
{
	char data = -1;
	int iOK = false;
	int iTimeout = 100;
	int iTimeoutBuf = 1000;
	int	iTime = 0;
	int iTick = 10;
	int numSetupTries = 5;

	while (numSetupTries > 0) {

		for (iTime = 0; iTime < iTimeoutBuf; iTime += iTick) {	// wait for full buffers to clear
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;			// or timeout iTimeoutBuf = 1000
			if (!(data & (EC_STAT_IBF | EC_STAT_OBF))) {
				iOK = true;
				break;
			}
			::Sleep(iTick);
		}

		if (!iOK) return 0;
		iOK = false;

		WritePort(EC_CTRLPORT, EC_CTRLPORT_WRITE);		// tell 'em we want to "WRITE"

		for (iTime = 0; iTime < iTimeout; iTime += iTick) { // wait for IBF and OBF to clear
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;
			if (!(data & (EC_STAT_IBF | EC_STAT_OBF))) {
				iOK = true;
				break;
			}
			::Sleep(iTick);
		}							// try again after a moment

		if (!iOK) return 0;
		iOK = false;

		WritePort(EC_DATAPORT, offset);					// tell 'em where we want to write to

		for (iTime = 0; iTime < iTimeout; iTime += iTick) { // wait for IBF and OBF to clear
			data = (char)ReadPort(EC_CTRLPORT) & 0xff;
			if (!(data & (EC_STAT_IBF | EC_STAT_OBF))) {
				iOK = true;
				numSetupTries = 1;
				break;
			}
			::Sleep(iTick);
		}							// try again after a moment
		numSetupTries -= 1;
	}
	if (!iOK) return 0;

	WritePort(EC_DATAPORT, NewData);				// tell 'em what we want to write there

	return 1;
}
