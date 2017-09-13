#include <Windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <conio.h>

#define IOCTL_TEST_CODE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OPEN_PROCESS_CODE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

SC_HANDLE g_scm_handle = NULL;
SC_HANDLE g_service_handle = NULL;

int __stdcall open_service_control_manager()
{
	g_scm_handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!g_scm_handle)
	{
		printf("open service control manager fail..\n");

		return 0;
	}

	return 1;
}

int __stdcall install_driver(wchar_t *service_name)
{
	wchar_t driver_name[MAX_PATH] = { 0, };

	GetCurrentDirectory(MAX_PATH, driver_name);
	StringCbCat(driver_name, MAX_PATH, L"\\");
	StringCbCat(driver_name, MAX_PATH, service_name);
	StringCbCat(driver_name, MAX_PATH, L".sys");

	printf("driver => %ls\n", driver_name);

	g_service_handle
		= CreateService
		(g_scm_handle
		, service_name
		, service_name
		, SERVICE_ALL_ACCESS
		, SERVICE_KERNEL_DRIVER
		, SERVICE_DEMAND_START
		, SERVICE_ERROR_NORMAL
		, driver_name, NULL, NULL, NULL, NULL, NULL);

	if (!g_service_handle)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS || GetLastError() == 0x430)
		{
			printf("open service!\n");

			g_service_handle = OpenService(g_scm_handle, service_name, SERVICE_ALL_ACCESS);

			if (!g_service_handle)
			{
				printf("open service err => %08x\n", GetLastError());

				CloseServiceHandle(g_scm_handle);

				return 0;
			}
		}
		else
		{
			printf("create service err => %08x\n", GetLastError());

			CloseServiceHandle(g_scm_handle);

			return 0;
		}
	}

	printf("service => %ls\n", service_name);

	return 1;
}

int __stdcall strart_driver()
{
	unsigned int s = StartService(g_service_handle, 0, NULL);
	SERVICE_STATUS service_status;

	if (!s)
	{
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("is running..\n");

			return 1;
		}
		else
		{
			printf("start service err => %08x\n", GetLastError());

			CloseServiceHandle(g_service_handle);
			CloseServiceHandle(g_scm_handle);

			return 0;
		}
	}

	printf("run driver!\n");

	return 1;
}

int __stdcall stop_driver()
{
	SERVICE_STATUS s;

	ControlService(g_service_handle, SERVICE_CONTROL_STOP, &s);

	if (s.dwCurrentState == 1)
		printf("stop service!\n");
	else
	{
		printf("stop service fail => %08x\n", s.dwCurrentState);

		//CloseServiceHandle(g_service_handle);
		//CloseServiceHandle(g_scm_handle);

		return 0;
	}

	return 1;
}

int __stdcall remove_driver()
{
	unsigned int s = DeleteService(g_service_handle);

	if (s)
	{
		printf("remov driver!!\n");

		CloseServiceHandle(g_service_handle);
		CloseServiceHandle(g_scm_handle);
	}
	else
	{
		printf("remov driver fail => %08x\n", GetLastError());

		CloseServiceHandle(g_service_handle);
		CloseServiceHandle(g_scm_handle);

		return 0;
	}

	return 1;
}

#if 1
void main()
{
	HANDLE h_device = NULL;
	HANDLE h_event = NULL;
	DWORD io_ret;
	BOOL s;

	if (!open_service_control_manager())
		return;

	if (!install_driver(L"kbmon"))
		return;

	if (!strart_driver())
		return;

	printf("driver is run!!\n");

	/*-- driver control test --*/
	h_device = CreateFile(L"\\\\.\\KBMON", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	//h_device = CreateFile(L"\\\\Device\\KBMON", GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (h_device == INVALID_HANDLE_VALUE)
		printf("get driver handle fail.. %08x\n", GetLastError());
	else
	{
		printf("get driver handle\n");

		if (!DeviceIoControl(h_device, (DWORD)IOCTL_TEST_CODE, 0, 0, 0, 0, &io_ret, 0))
			printf("dic fail.. %08x\n", GetLastError());

		ULONG pid = GetCurrentProcessId();
		HANDLE h_process = (PVOID)0x123;

		printf("open => %d\n\n", pid);

		if (!DeviceIoControl(h_device, (DWORD)IOCTL_OPEN_PROCESS_CODE, &pid, sizeof(pid), &h_process, sizeof(h_process), &io_ret, 0))
			printf("dic open process fail..\n");

		if (!h_process)
			printf("open process fail..\n");
		else
			printf("bmon:: open process => %08x\n", h_process);
	}
	/*-- driver control test end --*/

	getch();

	CloseHandle(h_device); // 1.

	if (!stop_driver())
		;

	remove_driver();

	CloseServiceHandle(g_scm_handle);
	CloseServiceHandle(g_service_handle);
}
#endif