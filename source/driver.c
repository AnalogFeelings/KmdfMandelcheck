#include <ntddk.h>
#include "bootvid.h"

//Limit allocated memory for the bitmap to 153.7 kilobytes.
#define BMP_BUFF_CAP 153718

//Visual Studio I swear to god, I have checked if "BugcheckCallbackRecord" is NULL.
//I even return if it is. SO WHY ARE YOU COMPLAINING THAT IT COULD BE NULL?
#pragma warning(disable: 6011 6387)

//---------------------------DRIVER FUNCTION DECLARATIONS---------------------------//
VOID						DriverUnload(PDRIVER_OBJECT DriverObj);
NTSTATUS					DriverEntry(PDRIVER_OBJECT DriverObj, PUNICODE_STRING RegistryPath);
NTSTATUS					ReadBitmapFile();
NTSTATUS					InitializeBugcheckCallback();
KBUGCHECK_CALLBACK_ROUTINE	BugcheckCallback;

//---------------------------DRIVER VARIABLE DECLARATIONS---------------------------//
PKBUGCHECK_CALLBACK_RECORD	BugcheckCallbackRecord;
UNICODE_STRING				BitmapFilePath = RTL_CONSTANT_STRING(L"\\DosDevices\\C:\\KmdfMandelcheck\\target.bmp");
PUCHAR						LoadedBitmapFile;
HANDLE						FileHandle;

//===================================================================================//

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObj, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("[%s:%d MSG] KmdfMandelcheck: Initializing driver.\n", __FILE__, __LINE__);

	DriverObj->DriverUnload = DriverUnload;

	NTSTATUS BitmapStatus = ReadBitmapFile();
	if(!NT_SUCCESS(BitmapStatus))
		return BitmapStatus;

	NTSTATUS CallbackStatus = InitializeBugcheckCallback();
	if(!NT_SUCCESS(CallbackStatus))
		return CallbackStatus;

	DbgPrint("[%s:%d MSG] KmdfMandelcheck: Driver initialized successfully!\n", __FILE__, __LINE__);

	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObj)
{
	UNREFERENCED_PARAMETER(DriverObj);

	DbgPrint("[%s:%d WRN] KmdfMandelcheck: Unloading driver.\n", __FILE__, __LINE__);

	KeDeregisterBugCheckCallback(BugcheckCallbackRecord);
	ExFreePoolWithTag(BugcheckCallbackRecord, 'BCHK');

	if(LoadedBitmapFile != NULL)
	{
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');
	}
}

NTSTATUS ReadBitmapFile()
{
	//Allocate the buffer. Must be nonpaged for it to be accessible inside the bugcheck
	//callback.
	LoadedBitmapFile = ExAllocatePoolZero(NonPagedPoolNx, BMP_BUFF_CAP, 'BTMP');
	if(LoadedBitmapFile == NULL)
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Could not allocate memory for bitmap.\n", __FILE__, __LINE__);

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, &BitmapFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	NTSTATUS FileStatus;
	IO_STATUS_BLOCK FileStatusBlock;

	//Open the file.
	FileStatus = ZwCreateFile(&FileHandle, GENERIC_READ, &ObjectAttributes,
								&FileStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL,
								0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT,
								NULL, 0);
	if(!NT_SUCCESS(FileStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Could not open target bitmap file. Status: %lX\n", __FILE__, __LINE__, FileStatus);
		ZwClose(FileHandle);
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	//Actually perform the read operation into the buffer.
	FileStatus = ZwReadFile(FileHandle, NULL, NULL,
							NULL, &FileStatusBlock, LoadedBitmapFile,
							BMP_BUFF_CAP, NULL, NULL);
	if(!NT_SUCCESS(FileStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Could not read target bitmap file. Status: %lX\n", __FILE__, __LINE__, FileStatus);
		ZwClose(FileHandle);
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	//Close handle to file. We don't need it anymore.
	ZwClose(FileHandle);

	return STATUS_SUCCESS;
}

NTSTATUS InitializeBugcheckCallback()
{
	//Allocate memory for the bugcheck callback record.
	//Must be nonpaged for it to work.
	BugcheckCallbackRecord = ExAllocatePoolZero(NonPagedPool, sizeof(KBUGCHECK_CALLBACK_RECORD), 'BCHK');
	if(BugcheckCallback == NULL)
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Could not allocate memory for the callback record.\n", __FILE__, __LINE__);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	KeInitializeCallbackRecord(BugcheckCallbackRecord);

	BOOLEAN Registered = KeRegisterBugCheckCallback(BugcheckCallbackRecord, BugcheckCallback, NULL, 0, (PUCHAR)"KmdfMandelcheck");
	if(!Registered)
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Could not register callback.\n", __FILE__, __LINE__);
		ExFreePoolWithTag(BugcheckCallbackRecord, 'BCHK');

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	return STATUS_SUCCESS;
}

//Initialize BOOTVID.DLL and blit the loaded bitmap to the screen.
//Then enter an infinite loop to prevent windows from automatically rebooting.
VOID BugcheckCallback(PVOID Buffer, ULONG Length)
{
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(Length);

	DbgPrint("[%s:%d WRN] KmdfMandelcheck: Bugcheck callback has been called!\n", __FILE__, __LINE__);

	VidInitialize(TRUE);
	VidResetDisplay(TRUE);
	VidSolidColorFill(0, 0, 639, 479, BV_COLOR_BLACK);

	//BOOTVID seems to not like it when you include the header.
	//Skip 14 bytes to remove it.
	VidBitBlt(&LoadedBitmapFile[0xE], 0, 0);

	DbgPrint("[%s:%d WRN] KmdfMandelcheck: Image has been drawn! Entering infinite loop...\n", __FILE__, __LINE__);

	for(;;)
		;
}