#include "command.hpp"
#include "usbmon.hpp"

#include <codecvt>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <libusb-1.0/libusb.h> // apt-get install libusb-1.0-0-dev
#include <locale>
#include <memory>
#include <string>
#include <unistd.h>

LuXact::LuXact(uint32_t vidpid)
  : id_int(vidpid)
{
	//initialize libusb session
	if (libusb_init(&lu_ctx) < 0)
		SysAbort(__PRETTY_FUNCTION__);
}

LuXact::~LuXact()
{
	Close();

	//terminate libusb session
	libusb_exit(lu_ctx);
}

bool LuXact::Open()
{
		libusb_device_handle* handle = libusb_open_device_with_vid_pid(lu_ctx, id_int >> 16, id_int & 0xFFFF);
		if (!handle)
			return false;

		// claim interface must succeed in order to own device
		if (libusb_kernel_driver_active(handle, 0) == 1
		  && libusb_detach_kernel_driver(handle, 0) != 0
		  || libusb_claim_interface(handle, 0) != 0)
		{
			libusb_close(lu_dev);
			return false;
		}
		lu_dev = handle;

	return true;
}

bool LuXact::Close()
{
	if (!lu_dev) return false;
	bool ret = true;

	if (libusb_release_interface(lu_dev, 0) != 0)
		ret = false;
	libusb_close(lu_dev);
	lu_dev = nullptr;

	return ret;
}

uint8_t LuXact::Xfer(uint8_t req, uint32_t arg, char* data, uint8_t size, bool send)
{
for (;;) {
	if (!lu_dev)
	{
		usleep(100 * 1000);
		if (!Open())
			continue;
	}
	uint8_t xfer = libusb_control_transfer(lu_dev,
	  /* bmRequestType */ (send ? LIBUSB_ENDPOINT_OUT : LIBUSB_ENDPOINT_IN) | LIBUSB_REQUEST_TYPE_CLASS,
	  /* bRequest */ req,
	  /* wValue */ (uint16_t)arg,
	  /* wIndex */ (uint16_t)(arg >> 16),
	  /* data */ (unsigned char*)data,
	  /* wLength */ size,
	  timeout);
	if (xfer >= 0 && xfer <= size)
		return xfer;
	Close();
}
}

bool LuXact::Send(const char* data)
{
	while (*data) try
	{
		Xfer(137, (uint8_t)*data++, nullptr, 0, true);
	} catch(...) {
		return true;
	}
	return false;
}

bool LuXact::Recv(char* data, uint8_t size)
{
	if (size < 4) throw Exception(__PRETTY_FUNCTION__);
	try {
		data[Xfer(137, 0, data, size)] = '\0';
	} catch(...) {
		return true;
	}
	return false;
}

void monitor()
{
	LuXact luXact(0x16d00753); //unuc
//	LuXact luXact(0x16c005df); //rpcusb
	luXact.Open();

	time_t last = time(nullptr);
	for (;;)
	{
		char data[9];
		if (!luXact.Recv(data, sizeof(data)-1) && data[0])
		{
			std::cout << data << std::flush;
			continue;
		}
		if (SysKbFull())
		{
			int c = SysKbRead();
			if (c < 0) continue;
			data[0] = c; data[1] = '\0';
			luXact.Send(data);
			continue;
		}

		usleep(1000 * 1000);
	}

}

void test()
{
	std::cerr << "test" << '\n';
	exit(-1);
}
