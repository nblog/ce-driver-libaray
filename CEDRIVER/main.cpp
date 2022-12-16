
#include <iostream>


#include "ce_driver.hpp"


EXTERN_C CONST IMAGE_DOS_HEADER __ImageBase;


int main()
{
	using namespace ce_driver_wrapper;

	ce_driver drv;

	/* AesopEngine AESOP64 */
	drv.load_driver(R"(C:\Public\runtime\aesop64.sys)", L"AESOP64");

	DWORD pid = 2072; 
	HANDLE hProcess = 0;
	UINT64 allocaddr = 0ull;


	/* open */
	{
		struct {
			DWORD processId;
		} in = { pid };
		request_ctl<decltype(in)> req{
			sizeof(in), in
		};

		struct {
			UINT64 Processhandle;
			UINT8 Special;
		} out = { };
		response_ctl<decltype(out)> res{
			sizeof(out), out
		};

		drv.device_io_control(ioctl::CE_OPENPROCESS, req, res);

		::printf_s("handle: %p\n", res.res.Processhandle);
	}

	/* alloc */
	{
		struct {
			UINT64 ProcessID;
			UINT64 BaseAddress;
			UINT64 Size;
			UINT64 AllocationType;
			UINT64 Protect;
		} in = { pid, 0, 100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE };
		request_ctl<decltype(in)> req{
			sizeof(in), in
		};

		struct {
			UINT64 BaseAddress;
		} out = { };
		response_ctl<decltype(out)> res{
			sizeof(out), out
		};

		drv.device_io_control(ioctl::CE_ALLOCATEMEM, req, res);

		::printf_s("base: %p\n", res.res.BaseAddress);

		allocaddr = res.res.BaseAddress;
	}

	/* write */
	{
		struct {
			UINT64 processid;
			UINT64 startaddress;
			WORD bytestowrite;
			CHAR padding[492];
		} in = { pid, allocaddr, 4 };
		request_ctl<decltype(in)> req{
			sizeof(in), in
		};

		response_ctl<decltype(in)> res{
			sizeof(in), in
		};

		strcpy_s(req.req.padding, "test");

		drv.device_io_control(ioctl::CE_WRITEMEMORY, req, res);

	}

	/* read */
	{
		struct {
			UINT64 processid;
			UINT64 startaddress;
			WORD bytestoread;
			CHAR padding[4078];
		} in = { pid, allocaddr, 4 };
		request_ctl<decltype(in)> req{
			sizeof(in), in
		};

		response_ctl<decltype(in)> res{
			sizeof(in), in
		};

		drv.device_io_control(ioctl::CE_READMEMORY, req, res);

		::printf_s("str: %s\n", &res.res);
	}
	
	::system("pause");

	return 0;
}
