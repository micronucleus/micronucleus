#include "command.hpp"
#include "usbmon.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

void SysAbort(const char* f, ...)
{
	if (f)
	{
		fprintf(stderr, "SysAbort: ");
		va_list a; va_start(a, f);
		vfprintf(stderr, f, a);
		va_end(a);
		fprintf(stderr, " ");
	}
	abort();
}

void SysAbort_unhandled()
{
	std::cerr << "SysAbort: ";
	std::exception_ptr ep = std::current_exception();
	try {
		std::rethrow_exception(ep);
	} catch (std::exception& ex) {
		const char* error = ex.what();
		std::cerr << (error ? error : "std::exception unhandled");
	} catch (...) {
		std::cerr << "... unhandled";
	}
	std::cerr << '\n';
	abort();
}

int SysKbFull()
{
	timeval tv = { 0, 0 };
	fd_set fds; FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
}

int SysKbRead()
{
	unsigned char c;
	int r = read(0, &c, sizeof(c));
	return r <= 0 ? -1 : (unsigned int)c;
}

static termios ttystate;
void reset_terminal_mode() { tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); }

int main(int argc, char *argv[])
{
	// get console settings
	tcgetattr(STDIN_FILENO, &ttystate);
	atexit(reset_terminal_mode);

{
	// set console noblock and noecho modes
	termios ttystate_new = ttystate;
	ttystate_new.c_lflag &= ~(ICANON | ECHO);
	ttystate.c_cc[VMIN] = 1; ttystate.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate_new);
}
	struct usage { };
try {
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			throw usage();
		else switch (argv[i][1])
		{
			default:
			throw usage();
			break;

			case 't':
			test();
			break;

		}
	}
} catch (usage) {
	std::cerr << R"00here-text00(usage: usbmon [-t]
)00here-text00";
	return 3;
}

	monitor();
	return 0;
}
