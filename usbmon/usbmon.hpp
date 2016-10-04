#ifndef usbmon_hpp
#define usbmon_hpp

#include <stdexcept>

class LuXact
{
	public:

	const uint32_t timeout = 10;

	class Exception : public std::runtime_error { public: Exception(const char* what_arg) : std::runtime_error(what_arg) { } };

	LuXact(uint32_t vidpid);
	~LuXact();

	bool Open();
	bool Close();
	uint8_t Xfer(uint8_t req, uint32_t arg, char* data = nullptr, uint8_t size = 0, bool send = false);

	bool Send(const char* data);
	bool Recv(char* data, uint8_t size);

	protected:

	uint32_t id_int;
	struct libusb_context* lu_ctx = nullptr;
	struct libusb_device_handle* lu_dev = nullptr;
};

void monitor();
void test();

#endif
