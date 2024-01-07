#include "main.h"


NTSTATUS InitializeDriver(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	auto  status = STATUS_SUCCESS;

	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path) {
	auto status = STATUS_SUCCESS;
	UNICODE_STRING  drv_name;

	RtlInitUnicodeString(&drv_name, L"\\Driver\\" DRIVER_NAME);
	status = IoCreateDriver(&drv_name, &InitializeDriver);

	return STATUS_SUCCESS;
}