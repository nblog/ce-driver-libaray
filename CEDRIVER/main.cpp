
#include <iostream>


#include "ce_driver.hpp"


EXTERN_C CONST IMAGE_DOS_HEADER __ImageBase;


int main()
{
	using namespace ce_driver_wrapper;

	ce_driver drv;

	/* AesopEngine AESOP64 */
	drv.load_driver(R"(C:\Public\runtime\aesop64.sys)", L"AESOP64");

	struct {
		DWORD processId;
	} in = { 8232 };
	request_ctl<decltype(in)> req{
		sizeof(in), in
	};
	
	struct {
		uint64_t Processhandle;
		uint8_t Special;
	} out = { };
	response_ctl<decltype(out)> res{
		sizeof(out), out
	};

	drv.device_io_control( ioctl::CE_OPENPROCESS, req, res );

	::printf_s("handle: %p\n", res.res.Processhandle);
	::system("pause");

	return 0;
}
