/* 
This driver is not used for anything yet.
*/

#include <ntddk.h>
#include <stdio.h>
#include "STKE.h"

#define DEVICE_NAME L"\\Device\\STKE"
#define SYM_LINK L"\\??\\STKE"

void stkeUnload(PDRIVER_OBJECT DriverObject);
DRIVER_DISPATCH stkeCreateClose, stkeDeviceControl;

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	UNICODE_STRING deviceName, symLink;
	RtlInitUnicodeString(&deviceName, DEVICE_NAME);
	RtlInitUnicodeString(&symLink, SYM_LINK);

	DriverObject->DriverUnload = stkeUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = stkeCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = stkeDeviceControl;

	PDEVICE_OBJECT DeviceObject = nullptr;
	auto status = STATUS_SUCCESS;
	auto symLinkCreated = false;

	do 
	{
		status = IoCreateDevice(DriverObject, sizeof(EXTRA_SPACE), &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to create device (0x%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symLink, &deviceName);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;
	} while (false);

	return status;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

void stkeUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink;
	RtlInitUnicodeString(&symLink, SYM_LINK);
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

