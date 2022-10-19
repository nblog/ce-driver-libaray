#pragma once

#include <filesystem>
namespace fs = std::filesystem;


#include <Windows.h>



#define IOCTL_UNKNOWN_BASE					FILE_DEVICE_UNKNOWN


namespace ce_driver_wrapper {

	/* https://github.com/cheat-engine/cheat-engine/blob/7.4/DBKKernel/IOPLDispatcher.h */
	enum class ioctl : uint32_t {
		CE_READMEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_WRITEMEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_OPENPROCESS = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_QUERY_VIRTUAL_MEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_GETPEPROCESS = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_OPENTHREAD = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0818, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_MAKEWRITABLE = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0819, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_ALLOCATEMEM = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x081f, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_CREATEAPC = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0820, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_GETPETHREAD = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0821, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_SUSPENDTHREAD = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0822, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_RESUMETHREAD = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0823, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_SUSPENDPROCESS = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0824, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_RESUMEPROCESS = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0825, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_GET_PEB = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x085d, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_QUERYINFORMATIONPROCESS = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x085e, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_NTPROTECTVIRTUALMEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x085f, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_LOCK_MEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0860, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_UNLOCK_MEMORY = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0861, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),

		CE_TEST = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
		CE_GETVERSION = CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0816, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
	};


	/* 
	* https://github.com/cheat-engine/cheat-engine/blob/7.4/Cheat%20Engine/dbk32/DBK32functions.pas
	* https://github.com/cheat-engine/cheat-engine/blob/7.4/DBKKernel/IOPLDispatcher.c#L336
	*/


	template <typename T>
	struct request_ctl {
		size_t size; T req;
	};

	template <typename T>
	struct response_ctl {
		size_t size; T res;
	};


	struct ce_driver {

		~ce_driver() {
			if (hDrv_) CloseHandle(hDrv_);
		}

		/* AesopEngine AESOP64 */
		const auto ce_service_name() const {
			return std::wstring(L"CEDRIVER73");
		}

		/* https://github.com/cheat-engine/cheat-engine/blob/7.4/DBKKernel/sigcheck.c */
		bool load_driver(const fs::path driver, const std::wstring service = std::wstring()) {
			bool isOk = false;

			auto ce_driver = driver.wstring();
			auto ce_service = service.empty() ? ce_service_name() : service;

			SC_HANDLE hManager = NULL, hService = NULL; HKEY hKeyService = NULL;

			hManager = OpenSCManagerW(NULL, NULL, GENERIC_READ | GENERIC_WRITE);
			if (!hManager) goto _cleanup;

			hService = OpenServiceW(hManager, ce_service.c_str(), SERVICE_ALL_ACCESS);
			if (!hService) goto _cleanup;

			ChangeServiceConfigW(
				hService, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				ce_driver.c_str(), NULL, NULL, NULL, NULL, NULL, ce_display_name().c_str());

			/* https://github.com/cheat-engine/cheat-engine/blob/7.4/DBKKernel/DBKDrvr.c#L237 */
			if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, ce_reg_path().c_str(), 0, KEY_ALL_ACCESS, &hKeyService)
				&& !hKeyService) 
				goto _cleanup;

			RegSetKeyValueW(hKeyService, nullptr, L"A", REG_SZ,
				driver_string().c_str(), DWORD((driver_string().length() + 1) * sizeof(driver_string()[0])) );
			RegSetKeyValueW(hKeyService, nullptr, L"B", REG_SZ,
				device_string().c_str(), DWORD((device_string().length() + 1) * sizeof(device_string()[0])));
			RegSetKeyValueW(hKeyService, nullptr, L"C", REG_SZ,
				process_event_string().c_str(), DWORD((process_event_string().length() + 1) * sizeof(process_event_string()[0])));
			RegSetKeyValueW(hKeyService, nullptr, L"D", REG_SZ,
				thread_event_string().c_str(), DWORD((thread_event_string().length() + 1) * sizeof(thread_event_string()[0])));

			/* stop service */
			{
				SERVICE_STATUS service_status = { };

				ControlService(hService, SERVICE_QUERY_STATUS, &service_status);

				if (SERVICE_RUNNING == service_status.dwCurrentState)
					ControlService(hService, SERVICE_CONTROL_STOP, &service_status);
			}

			/* start service */
			if (StartServiceW(hService, 0, NULL)
				&& (hDrv_ = CreateFileW(ce_object_path().c_str(),
					GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL))) {
				isOk = true;
			}

		_cleanup:
			
			if (hKeyService) {
				RegDeleteValueW(hKeyService, L"A");
				RegDeleteValueW(hKeyService, L"B");
				RegDeleteValueW(hKeyService, L"C");
				RegDeleteValueW(hKeyService, L"D");
			}

			if (hKeyService) RegCloseKey(hKeyService);

			if (hService) CloseServiceHandle(hService);

			if (hManager) CloseServiceHandle(hManager);

			return isOk;
		}


		template <typename req, typename res>
		bool device_io_control(ioctl ctl, request_ctl<req>& in, response_ctl<res>& out) {
			DWORD rtBytes = 0;
			return DeviceIoControl(
				HANDLE(hDrv_), DWORD(ctl),
				&in.req, DWORD(in.size), 
				&out.res, DWORD(out.size), 
				&rtBytes, NULL);
		}

	private:
		const std::wstring ce_display_name() const {
			return ce_service_name();
		}
		const std::wstring ce_object_path() const {
			return L"\\\\.\\" + ce_service_name();
		};
		const std::wstring driver_string() const {
			return L"\\Device\\" + ce_service_name();
		};
		const std::wstring device_string() const {
			return L"\\DosDevices\\" + ce_service_name();
		};
		const std::wstring process_event_string() const {
			return L"\\BaseNamedObjects\\" + std::wstring(L"DBKProcList60");
		};
		const std::wstring thread_event_string() const {
			return  L"\\BaseNamedObjects\\" + std::wstring(L"DBKThreadList60");
		};
		const std::wstring ce_reg_path() const {
			return L"SYSTEM\\CurrentControlSet\\Services\\" + ce_service_name();
		}

		HANDLE hDrv_ = INVALID_HANDLE_VALUE;
	};
}


#undef IOCTL_UNKNOWN_BASE