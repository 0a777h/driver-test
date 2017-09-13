#include <ntifs.h>

#include <ntddk.h>
#include <wdm.h>

NTSTATUS KbmOpenProcess(PIRP irp, PHANDLE h_process)
{
	PEPROCESS eprocess;
	ULONG process_id;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(eprocess);
	UNREFERENCED_PARAMETER(status);
	UNREFERENCED_PARAMETER(h_process);
	
	__try
	{
		process_id = *(PULONG)irp->AssociatedIrp.SystemBuffer;
		status = PsLookupProcessByProcessId((HANDLE)process_id, &eprocess);

		UNREFERENCED_PARAMETER(status);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		status = ObOpenObjectByPointer(eprocess, 0, NULL, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, h_process);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		DbgPrint("open process => %d\n", process_id);
	}
	__except (1)
	{
		DbgPrint("KbmOpenProcess crash..\n");
		status = STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}