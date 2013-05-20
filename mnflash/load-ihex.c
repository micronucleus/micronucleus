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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "firmware-blob.h"
#include "load-ihex.h"


static int8_t char2nibble(char nibble)
{
	if ( nibble >= '0' && nibble <= '9') {
		return nibble - '0';
	}
	else if ( nibble >= 'A' && nibble <= 'F' ) {
		return nibble - 'A' + 10;
	}
	else if ( nibble >= 'a' && nibble <= 'f' ) {
		return nibble - 'a' + 10;
	}
	else {
		return -1;
	}
}

static int hex2byte(char * hexstr, uint8_t * byte)
{
	int8_t n1, n2;

	if ( ((n1 = char2nibble(*hexstr)) >= 0 ) && ((n2 = char2nibble(*(hexstr + 1))) >= 0) ) {
		*byte = ((n1 << 4) + n2);
		return 2;
	}
	return 0;
}

static int hex2word(char * hexstr, uint16_t * word)
{
	uint8_t b1, b2;

	if ( ((hex2byte(hexstr, &b1)) >= 0 ) && (hex2byte(hexstr + 2, &b2) >= 0) ) {
		*word= ((b1 << 8) + b2);
		return 1;
	}
	return 0;
}

struct firmware_blob * ihex_load(const char * filename)
{
	FILE * in = NULL;
	struct firmware_blob * blob = NULL;
	char  linebuf[256];

	if ( (in = fopen( filename, "r")) == NULL ) {
		fprintf(stderr, "cannot open %s: %s\n", filename, strerror(errno));
		return NULL;
	}

	blob = firmware_blob_new();
	blob->filename = strdup(filename);

	while( fgets(linebuf, sizeof(linebuf), in) != NULL) {
		size_t slen = strlen(linebuf);

		uint8_t  rec_len;
		uint16_t rec_address;
		uint8_t  rec_type;
		size_t   addr_offset   = 0;

		/* "chomp" - remove <cr><lf> from the string just read. */
		if ( linebuf[slen - 1] == '\n' ) {
			slen --;
			linebuf[slen] = 0;
		}

		if ( linebuf[slen - 1] == '\r' ) {
			slen --;
			linebuf[slen] = 0;
		}

		if ( slen < 9 ) {
			fprintf(stderr, "firmware record too short.\n");
			goto errout;
		}

		if ( *linebuf != ':' ) {
			fprintf(stderr, "Invalid record start marker.\n");
			goto errout;
		}

		if ( hex2byte(linebuf + 1, &rec_len) <= 0 ) {
			fprintf(stderr, "cannot parse record data len.\n");
			goto errout;
		}

		if ( slen < 9 + rec_len ) {
			fprintf(stderr, "record seems to be corrupted.\n");
			goto errout;
		}

		//fprintf(stderr, "slen=%zd rec_len=%d\n", slen, rec_len);
		if ( slen == 11 + rec_len * 2 ) {
			int    i = 0;
			int    sum = 0;
			int8_t sum_lsb;

			for ( i = 1; i < (rec_len * 2 + 10); i += 2 ) {
				uint8_t byte;
				if ( hex2byte(linebuf + i, &byte) <= 0 ) {
					fprintf(stderr, "cannot parse bytes in line\n");
					goto errout;
				}
				sum += byte;
			}

			sum_lsb = sum & 0xFF;
			sum_lsb ^= 0xFF;
			sum_lsb += 1;

			// fprintf(stderr, "checksum is %2.2x\n", sum_lsb);
			if ( sum_lsb != 0 ) {
				fprintf(stderr, "checksum error\n");
				goto errout;
			}
		}

		if ( hex2word(linebuf + 3, &rec_address) <= 0 ) {
			fprintf(stderr, "cannot parse record address.\n");
			goto errout;
		}

		if ( hex2byte(linebuf + 7, &rec_type) <= 0 ) {
			fprintf(stderr, "cannot parse record data len.\n");
			goto errout;
		}

		switch( rec_type ) {
			case 0: {
				/* firmware block */
				int i = 0;
				size_t addr = addr_offset + rec_address;

				if ((blob->data == NULL) || (addr < blob->start))
					blob->start = addr;

				firmware_blob_resize(blob, addr + rec_len);

				if ( blob->data == NULL ) {
					fprintf(stderr, "blob resize failed\n");
					goto errout;
				}

				for ( i = 0; i < rec_len; i ++ ) {
					hex2byte(linebuf + i*2 + 9, blob->data + addr);
					addr ++;
				}
				blob->end = addr;

				break;
			}
			case 1:
				/* end-of-file + start address */
				if ( blob->entry == 0 )
					blob->entry = rec_address;
				break;
			case 2: {
				/* Extended segment address record */
				uint16_t segment = 0;
				if ( hex2word(linebuf + 9, &segment) > 0) {
					addr_offset = segment * 16;
				} else {
					fprintf(stderr, "error parsing extended segment address record (2)\n");
					goto errout;
				}
				break;
			}
			case 3: {
				/* start segment address record */
				uint16_t segment = 0;
				uint16_t offset  = 0;
				if ( (hex2word(linebuf + 9, &segment) > 0) && (hex2word(linebuf + 13, &offset) > 0)) {
					blob->entry = segment * 16 + offset;
				} else {
					fprintf(stderr, "error parsing start segment address record (3)\n");
					goto errout;
				}
				break;
			}
			case 4: {
				/* extended linear address record */
				uint16_t segment = 0;
				if ( hex2word(linebuf + 9, &segment) > 0) {
					addr_offset = segment << 16;
				} else {
					fprintf(stderr, "error parsing extended linear address record (4)\n");
					goto errout;
				}
				break;
			}
			case 5: {
				/* start linear address record */
				uint16_t offset  = 0;
				uint16_t segment = 0;
				if ( (hex2word(linebuf + 9, &segment) > 0) && (hex2word(linebuf + 13, &offset) > 0)) {
					blob->entry = (segment << 16) + offset;
				} else {
					fprintf(stderr, "error parsing start linear address record (5)\n");
					goto errout;
				}
				break;
			}
			default:
				fprintf(stderr, "Invalid record type '%d'\n", rec_type);
				goto errout;

		}

		// fprintf(stderr, "%s\n", linebuf);
		// fprintf(stderr, "len = %d, addr = %x, type = %d\n", rec_len, rec_address, rec_type);
	}

	fclose(in);
	return blob;

errout:
	firmware_blob_destroy(blob);
	fclose(in);
	return NULL;
}


