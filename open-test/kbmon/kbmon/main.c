#include <ntddk.h>
#include <wdm.h>

#include "ctl_process.h"

#define IOCTL_TEST_CODE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OPEN_PROCESS_CODE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

UNICODE_STRING device_name;
UNICODE_STRING device_link;
PDEVICE_OBJECT g_device_object;

NTSTATUS StubFunction(PDEVICE_OBJECT device_object, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS KbIoControl(PDEVICE_OBJECT device_object, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	PIO_STACK_LOCATION io_stack_location;
	ULONG func_code;
	/*-- --*/
	HANDLE h_process = NULL;

	UNREFERENCED_PARAMETER(h_process);

	io_stack_location = IoGetCurrentIrpStackLocation(irp);
	func_code = io_stack_location->Parameters.DeviceIoControl.IoControlCode;

	DbgPrint("kbmon:: io control %d", func_code);

	switch (func_code)
	{
	case IOCTL_TEST_CODE:
		DbgPrint("kbmon:: io test code\n");
		break;

	case IOCTL_OPEN_PROCESS_CODE:
		DbgPrint("kbmon:: open process code\n");
		//DbgPrint("kbmon:: test %d\n", *(PULONG)io_stack_location->Parameters.DeviceIoControl.Type3InputBuffer);
		DbgPrint("kbmon:: test %08x\n", *(PULONG)irp->UserBuffer);
		if (NT_SUCCESS(KbmOpenProcess(irp, &h_process)))
		{
			DbgPrint("kbmon:: open process %08x\n", h_process);
			*(PULONG64)irp->UserBuffer = (ULONG64)h_process;
		}
		else
			*(PULONG64)irp->UserBuffer = 0xffffffff;
		break;

	default:
		break;
	}

	/*-- unload를 위하여 설정  --*/
	irp->IoStatus.Status = STATUS_SUCCESS;

	if (io_stack_location)
	{
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	return STATUS_SUCCESS;
}

NTSTATUS DriverUnload(
	_In_ PDRIVER_OBJECT driver_object
	)
{
	UNREFERENCED_PARAMETER(driver_object);

	IoDeleteDevice(g_device_object);
	//IoDeleteDevice(driver_object->DeviceObject);
	IoDeleteSymbolicLink(&device_link);

	DbgPrint("kbmon unload!\n");

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT		driver_object,
	_In_ PUNICODE_STRING	registry_path
	)
{
	NTSTATUS status;
	int init_index;

	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	DbgPrint("install kbmon.\n");

	RtlInitUnicodeString(&device_name, L"\\Device\\KBMON");
	status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, 0, TRUE, &g_device_object);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("f create device\n");
		return status;
	}

	RtlInitUnicodeString(&device_link, L"\\DosDevices\\KBMON");
	status = IoCreateSymbolicLink(&device_link, &device_name);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("f create symbol link\n");
		return status;
	}

	for (init_index = 0; init_index < IRP_MJ_MAXIMUM_FUNCTION; ++init_index)
	{
		driver_object->MajorFunction[init_index] = StubFunction;
	}

	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KbIoControl;
	driver_object->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}
