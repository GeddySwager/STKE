/* 
This driver is not used for anything yet.
*/

#include <ntddk.h>
#include <stdio.h>
#include "STKE.h"

#define IOCTL_STKE_LOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEVICE_NAME L"\\Device\\STKE"
#define SYM_LINK L"\\??\\STKE"

void stkeUnload(PDRIVER_OBJECT DriverObject);
DRIVER_DISPATCH stkeCreateClose, stkeDeviceControl;

DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	UNICODE_STRING deviceName, symLink;
	RtlInitUnicodeString(&deviceName, DEVICE_NAME);
	RtlInitUnicodeString(&symLink, SYM_LINK);

	DriverObject->DriverUnload = stkeUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = stkeCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = stkeDeviceControl;

	PDEVICE_OBJECT DeviceObject = NULL;
	auto status = STATUS_SUCCESS;
	auto symLinkCreated = FALSE;

	do 
	{
		status = IoCreateDevice(DriverObject, sizeof(UNICODE_STRING), &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
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
		symLinkCreated = TRUE;
	} while (FALSE);

	return status;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status , ULONG_PTR info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

NTSTATUS stkeCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) 
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_SUCCESS;
	return CompleteIrp(Irp, status, 0);
}

void stkeUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink;
	RtlInitUnicodeString(&symLink, SYM_LINK);
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS stkeDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	
	if (stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STKE_LOAD_DRIVER)
	{
		// TODO: implement ZwLoadDriver
		UNICODE_STRING driverName;
		RtlInitUnicodeString(&driverName, Irp->AssociatedIrp.SystemBuffer);

		NTSTATUS status = ZwLoadDriver(&driverName);
		return CompleteIrp(Irp, status, 0);
	}
	else
	{
		return CompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
	}

}