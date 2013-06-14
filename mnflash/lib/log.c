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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libmnflash/log.h>

static void log_stdout(const char * msg)
{
	fprintf(stdout, "%s\n", msg);
}

static void log_stderr(const char * msg)
{
	fprintf(stderr, "%s\n", msg);
}

static struct {
	mnflash_logfunc_t	logout;
	mnflash_logfunc_t	errout;
	void *			progress;
} mnflash_output = {
	.logout = log_stdout,
	.errout = log_stderr
};

void mnflash_set_error_log(mnflash_logfunc_t newlog)
{
	mnflash_output.errout = newlog;
}

void mnflash_set_message_log(mnflash_logfunc_t newlog)
{
	mnflash_output.logout = newlog;
}

int mnflash_msg(char *fmt, ...)
{
	char * msg = NULL;
	int mlen = 0;

	va_list argptr;
	va_start(argptr,fmt);
	mlen = vasprintf(&msg,fmt,argptr);
	va_end(argptr);

	if ( msg && mnflash_output.errout ) {
		mnflash_output.logout(msg);
		free(msg);
	}

	return mlen;
}

int mnflash_error(char *fmt, ...)
{
	char * msg = NULL;
	int mlen = 0;

	va_list argptr;
	va_start(argptr,fmt);
	mlen = vasprintf(&msg,fmt,argptr);
	va_end(argptr);

	if ( msg && mnflash_output.errout ) {
		mnflash_output.errout(msg);
		free(msg);
	}

	return mlen;
}

