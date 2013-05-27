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
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libmnflash/firmware.h>
#include <libmnflash/load-raw.h>

#define CHECKINTS 15
#define BUFSIZE 4096

mnflash_firmware_t * mnflash_raw_load(const char * filename)
{
	int in = 0;
	mnflash_firmware_t * blob = NULL;
	uint8_t buffer[BUFSIZE];
	size_t  now = 0;
	ssize_t bytesread = 0;

	if ( (in = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "cannot open %s: %s\n", filename, strerror(errno));
		return NULL;
	}

	blob = mnflash_firmware_new();
	blob->filename = strdup(filename);

	while ((bytesread = read(in,buffer,sizeof(buffer))) > 0) {

		mnflash_firmware_resize(blob, now + bytesread);

		if ( blob->data == NULL ) {
			fprintf(stderr, "blob resize failed\n");
			goto errout;
		}

		if (now == 0) {
			int i = 0;
			if ( bytesread < (2 * CHECKINTS) ) {
				fprintf(stderr, "raw file too short\n");
				goto errout;
			}

			if ( (buffer[3] & 0xF0) == 0xC0 ) {
				// rjmp every 4 bytes ?
				for ( i = 1; i < (2 * CHECKINTS); i += 2 ) {
					if ( ((buffer[i] & 0xF0) != 0xC0 ) ) {
						fprintf(stderr, "raw file does not seem to contain an interrupt table\n");
						goto errout;
					}
				}
			} else {
				/* raw-file for 16k devices? */
				if ( bytesread < (4 * CHECKINTS) ) {
					fprintf(stderr, "raw file too short\n");
					goto errout;
				}
				for ( i = 0; i < (4 * CHECKINTS); i += 4 ) {
					// jmp or rjmp every 4 bytes ?
					if ( ((buffer[i+1] & 0xF0) != 0xC0 ) ||
					     !(buffer[i] == 0x0c && buffer[i+1] == 0x94))
					{
						fprintf(stderr, "raw file does not seem to contain an interrupt table\n");
						goto errout;
					}
				}
			}

		}

		memcpy(blob->data + now, buffer, bytesread);
		blob->start = 0;
		now += bytesread;
		blob->end = now;

	}
	close(in);
	return blob;

errout:
	mnflash_firmware_destroy(blob);
	close(in);
	return NULL;
}


