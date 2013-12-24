/*
 *   Created: May 2013
 *   by Andreas Hofmeister <andi@collax.com>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of
 *   this software and associated documentation files (the "Software"), to deal in
 *   the Software without restriction, including without limitation the rights to
 *   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *   of the Software, and to permit persons to whom the Software is furnished to do
 *   so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <libmnflash/hexdump.h>
#include <libmnflash/log.h>

void mnflash_hexdump_header()
{
	mnflash_msg( "Addr  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F       ASCII");
	mnflash_msg( "-------------------------------------------------------------------------");
}

void mnflash_hexdump(int start, uint8_t * buffer, size_t buflen)
{
	int addr = start;
	char ascii[17];
	char linebuf[81];
	int first = 1;

	memset(linebuf, 0, sizeof(linebuf));
	memset(ascii,' ',sizeof(ascii) - 1);
	ascii[sizeof(ascii) - 1] = 0;

	if ( (start % 16) != 0 ) {
		addr = ((start / 16) * 16);

		snprintf(linebuf, sizeof(linebuf), "%4.4x ", addr);
		while ( addr < start ) {
			if (( addr % 8) == 0 )
				strncat(linebuf," ",sizeof(linebuf) - strlen(linebuf) - 1);
			strncat(linebuf,"..",sizeof(linebuf) - strlen(linebuf) - 1 );
			addr ++;
		}
	}

	while ( addr - start < buflen ) {
		unsigned char c = buffer[addr - start];
		char bb[4];

		if ( (addr % 16) == 0 ) {
			if ( first ) {
				snprintf(linebuf, sizeof(linebuf), "%4.4x ", addr);
			} else {
				strncat(linebuf, " ",   sizeof(linebuf) - strlen(linebuf) - 1);
				strncat(linebuf, ascii, sizeof(linebuf) - strlen(linebuf) - 1);
				mnflash_msg(linebuf);
				memset(linebuf, 0, sizeof(linebuf));
				snprintf(linebuf, sizeof(linebuf), "%4.4x ", addr);
			}
			memset(ascii,' ',sizeof(ascii) - 1);
		}
		else if ( (addr % 8) == 0 ) {
			strncat(linebuf, " ", sizeof(linebuf) - strlen(linebuf) - 1);
		}

		snprintf(bb, sizeof(bb), "%2.2x ", c );
		strncat(linebuf, bb, sizeof(linebuf) - strlen(linebuf) - 1);
		if ( isprint(c) ) {
			ascii[addr % 16] = c;
		} else {
			ascii[addr % 16] = '.';
		}
		addr ++;
		first = 0;
	}

	while ( (addr % 16) != 0 ) {
		strncat(linebuf,"   ",sizeof(linebuf) - strlen(linebuf) - 1);
		addr ++;
	}

	strncat(linebuf, " ", sizeof(linebuf) - strlen(linebuf) - 1);
	strncat(linebuf, ascii, sizeof(linebuf) - strlen(linebuf) - 1);
	mnflash_msg(linebuf);
}

