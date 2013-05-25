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

#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hexdump.h"
#include "firmware-blob.h"


static int elf_add_section_to_blob(struct firmware_blob * blob, Elf_Scn * section, ssize_t start)
{
	GElf_Shdr shdr;
	Elf_Data  *data = NULL;

	if (gelf_getshdr(section, &shdr) != &shdr)
		return 1;

	if (start < 0)
		start = blob->end;

	firmware_blob_resize(blob, start + shdr.sh_size);

	while ((data = elf_getdata(section, data)) != NULL) {
		memcpy(blob->data + blob->end, data->d_buf, data->d_size);
		blob->end += data->d_size;
	}

	return 0;
}

#define erret(msg ...) { fprintf(stderr, msg); goto errout; }

const char * sections[] = {
	".text",
	".data",
	".payload",
	".payloadvectors",
	".signature",
	NULL
};

struct firmware_blob * elf_load(const char *filename)
{
	int i, fd;
	struct firmware_blob * blob = NULL;
	const char ** wanted = NULL;

	Elf *e = NULL;
	Elf_Scn *scn = NULL;

	GElf_Ehdr ehdr;
	GElf_Shdr shdr;

	size_t shstrndx;

	if ((fd = open(filename, O_RDONLY, 0)) < 0)
		erret("open \%s\" failed", filename);

	if (elf_version(EV_CURRENT) == EV_NONE)
		erret("ELF library initialization failed: %s\n", elf_errmsg(-1));

	if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
		erret("elf_begin() failed: %s.\n", elf_errmsg(-1));

	if (elf_kind(e) != ELF_K_ELF)
		erret("%s is not an ELF object.\n", filename);

	if ((i = gelf_getclass(e)) == ELFCLASSNONE)
		erret("getclass() failed: %s.\n", elf_errmsg(-1));

	if (gelf_getehdr(e, &ehdr) == NULL)
		erret("getehdr() failed: %s.\n", elf_errmsg(-1));

	if (ehdr.e_machine != EM_AVR)
		erret("Wrong machine type.\n");

	if (ehdr.e_type != ET_EXEC)
		erret("ELF file is not an executable.\n");

	/*
	 * Find .text and .data sections
	 */
	if (elf_getshdrstrndx(e, &shstrndx) != 0)
		erret("getshstrndx() failed: %s.\n", elf_errmsg(-1));

	if ((blob = firmware_blob_new()) == NULL )
		erret("cannot create a new firmware blob\n");

	blob->filename = strdup(filename);

	/*
	 * FIXME: (or more, investigate me)
	 *
	 *  There must be a better way to find sections.
	 *
	 */
	for (wanted = sections; *wanted; wanted ++) {
		// fprintf(stderr,"looking for section %s\n", *wanted);
		scn = NULL;
		while ((scn = elf_nextscn(e, scn)) != NULL) {
			char *name = NULL;

			if (gelf_getshdr(scn, &shdr) != &shdr)
				erret("getshdr() failed: %s.\n", elf_errmsg(-1));

			if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
				erret("elf_strptr() failed: %s.\n", elf_errmsg(-1));

			if ( strcmp(*wanted, name) != 0 )
				continue;

			if ( shdr.sh_size <= 0 )
				continue;

			if (strcmp(name, ".text") == 0) {
				if (shdr.sh_type != SHT_PROGBITS)
					erret(".text is not a PROGBITS section\n");

				if ((shdr.sh_flags & SHF_EXECINSTR) == 0)
					erret(".text does not contain executable content\n");

				blob->start = blob->end = shdr.sh_addr;
				elf_add_section_to_blob(blob, scn, shdr.sh_addr);
			}
			else if (strcmp(name, ".data") == 0) {
				if (shdr.sh_type != SHT_PROGBITS)
					erret(".data is not a PROGBITS section\n");

				if ((shdr.sh_flags & SHF_EXECINSTR) != 0)
					erret(".data is executable\n");

				elf_add_section_to_blob(blob, scn, -1 );
			}
			else if ((strcmp(name, ".payload") == 0) || (strcmp(name, ".payloadvectors") == 0)) {
				if (shdr.sh_type != SHT_PROGBITS)
					erret(".payload is not a PROGBITS section\n");

				if ((shdr.sh_flags & SHF_EXECINSTR) != 0)
					erret(".payload is executable\n");

				elf_add_section_to_blob(blob, scn, shdr.sh_addr);
			}
			else if (strcmp(name, ".signature") == 0) {
				Elf_Data  *data = NULL;

				if (shdr.sh_type != SHT_PROGBITS)
					erret(".text is not a PROGBITS section\n");

				if (shdr.sh_size != 3)
					erret(".signature must be 3 bytes long\n");

				if ((data = elf_getdata(scn, data)) == NULL)
					erret("cannot get data from .signature\n");

				memcpy(blob->signature,data->d_buf, 3);
			}
			else {
				continue;
			}

			/*
			printf("Section %-4.4jd %s\n", elf_ndxscn(scn), name);
			printf("  type: %d\n", shdr.sh_type);
			printf("  size: %zd\n", shdr.sh_size);
			printf("  addr: 0x%zx\n", shdr.sh_addr);
			printf("offset: 0x%zx\n", shdr.sh_offset);
			printf(" flags: 0x%zx\n", shdr.sh_flags);
			*/
		}
	}

	elf_end(e);
	close(fd);

	return blob;

errout:
	if ( fd )
		close(fd);
	if ( blob )
		firmware_blob_destroy(blob);
	if ( e )
		elf_end(e);

	return NULL;
}

