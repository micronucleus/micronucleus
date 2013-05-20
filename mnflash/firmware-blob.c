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

#include "hexdump.h"
#include "firmware-blob.h"

struct firmware_blob * firmware_blob_new(void)
{
	struct firmware_blob * blob;

	if ( (blob = malloc(sizeof(struct firmware_blob))) != NULL ) {
		memset(blob,0,sizeof(*blob));
	}

	return blob;
}

void firmware_blob_destroy(struct firmware_blob * blob)
{
	if (blob) {
		free(blob->data);
		free(blob->filename);
	}

	free(blob);
}

#define FIRMWARE_ALLOC_SIZE	4096
#define FIRMWARE_MAX_DATA_SIZE	(1024 * 4096)

void firmware_blob_resize(struct firmware_blob * blob, size_t address) {
	if ( blob->data_size < address || (blob->data == NULL) ) {
		size_t new_alloc = ((address / FIRMWARE_ALLOC_SIZE) + 1) * FIRMWARE_ALLOC_SIZE;

		if ( new_alloc > FIRMWARE_MAX_DATA_SIZE ) {
			fprintf(stderr,"firmware image size exceeds %d bytes\n", FIRMWARE_MAX_DATA_SIZE);
			free(blob->data);
			blob->data = NULL;
			return;
		}

		// fprintf(stderr, "Re-alloc firmware blob to %zu bytes\n", new_alloc);
		blob->data = realloc(blob->data, new_alloc);

		/* fill new area with 0xFF = unitialized flash */
		memset( blob->data + blob->data_size, 0xFF, new_alloc - blob->data_size );

		blob->data_size = new_alloc;
	}
}

void firmware_info(const struct firmware_blob * blob)
{
	fprintf(stdout, "       File name: %s\n", blob->filename);
	fprintf(stdout, "Target Signature: 0x%2.2x 0x%2.2x 0x%2.2x\n",
			blob->signature[2], blob->signature[1], blob->signature[0]);
	fprintf(stdout, "            Size: %zd bytes\n", blob->end - blob->start);
	fprintf(stdout, " Load to address: 0x%zx\n", blob->start);
	fprintf(stdout, "   Start address: 0x%zx\n", blob->entry);
}

void firmware_dump(const struct firmware_blob * blob)
{
	firmware_info(blob);
	hexdump_header();
	hexdump(blob->start, blob->data + blob->start, blob->end - blob->start);
	// hexdump(0, blob->data, blob->data_size );


}


