#include "main.h"

namespace data {
	HANDLE processId;
	PEPROCESS targetProcess;
}

void init_process(PIRP irp) {
	p_init pInitData = (p_init)irp->AssociatedIrp.SystemBuffer;
	if (pInitData) {
		data::processId = (HANDLE)pInitData->processId;

		pInitData->result = PsLookupProcessByProcessId(data::processId, &data::targetProcess);
	}
}

NTSTATUS ReadVirtualMemory(PEPROCESS process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size, PSIZE_T ReadedBytes) {
	__try {
		MmCopyVirtualMemory(data::targetProcess, SourceAddress, process, TargetAddress, Size, KernelMode, ReadedBytes);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_ACCESS_VIOLATION;
	}
	return STATUS_SUCCESS;
}

NTSTATUS ctl_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	ULONG informationSize = 0;
	auto stack = IoGetCurrentIrpStackLocation(irp);

	const ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	size_t size = 0;

	p_read pReadRequest;
	p_write pWriteRequest;

	switch (controlCode) {
	case IO_INIT_REQUEST:
		init_process(irp);
		informationSize = sizeof(init);
		break;
	case IO_READ_REQUEST:
		pReadRequest = (p_read)irp->AssociatedIrp.SystemBuffer;

		if (pReadRequest) {
			if (pReadRequest->address < 0x7FFFFFFFFFFF) {
				SIZE_T bytes;
				pReadRequest->result = ReadVirtualMemory(PsGetCurrentProcess(), (PVOID)pReadRequest->address, pReadRequest->response, pReadRequest->size, &bytes);
			}
			else {
				pReadRequest->response = nullptr;
				pReadRequest->result = STATUS_ACCESS_VIOLATION;
			}
		}
		informationSize = sizeof(read);
		break;
	}

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = informationSize;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS InitializeDriver(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	auto  status = STATUS_SUCCESS;
	UNICODE_STRING  sym_link, dev_name;
	PDEVICE_OBJECT  dev_obj;

	RtlInitUnicodeString(&dev_name, L"\\Device\\" DRIVER_NAME);
	status = IoCreateDevice(driver_obj, 0, &dev_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);

	if (status != STATUS_SUCCESS) {
		return status;
	}

	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\" DRIVER_NAME);
	status = IoCreateSymbolicLink(&sym_link, &dev_name);

	if (status != STATUS_SUCCESS) {
		return status;
	}

	dev_obj->Flags |= DO_BUFFERED_IO;

	for (int t = 0; t <= IRP_MJ_MAXIMUM_FUNCTION; t++)
		driver_obj->MajorFunction[t] = unsupported_io;

	driver_obj->MajorFunction[IRP_MJ_CREATE] = create_io;
	driver_obj->MajorFunction[IRP_MJ_CLOSE] = close_io;
	driver_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ctl_io;
	driver_obj->DriverUnload = NULL;

	dev_obj->Flags &= ~DO_DEVICE_INITIALIZING;

	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	auto status = STATUS_SUCCESS;
	UNICODE_STRING  drv_name;

	RtlInitUnicodeString(&drv_name, L"\\Driver\\" DRIVER_NAME);
	status = IoCreateDriver(&drv_name, &InitializeDriver);

	return STATUS_SUCCESS;
}