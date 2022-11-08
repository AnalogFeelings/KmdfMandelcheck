#include "driver.h"

//---------------------------DRIVER FUNCTION DECLARATIONS---------------------------//
NTSTATUS							DriverEntry(PDRIVER_OBJECT DriverObj, PUNICODE_STRING RegistryPath);
VOID								DriverUnload(PDRIVER_OBJECT DriverObj);

NTSTATUS							GetKernelBaseAddress(PDRIVER_OBJECT DriverObj);
NTSTATUS							ReadBitmapFile(VOID);
NTSTATUS							InitializeBugcheckCallback(VOID);

KBUGCHECK_CALLBACK_ROUTINE			BugcheckCallback;

//---------------------------DRIVER VARIABLE DECLARATIONS---------------------------//
PKBUGCHECK_CALLBACK_RECORD			BugcheckCallbackRecord = { 0 };

PVOID								KernelBaseAddress = NULL;
UNICODE_STRING						KernelFileName = RTL_CONSTANT_STRING(L"ntoskrnl.exe");

UNICODE_STRING						BitmapFilePath = RTL_CONSTANT_STRING(L"\\DosDevices\\C:\\KmdfMandelcheck\\target.bmp");
PUCHAR								LoadedBitmapFile;
HANDLE								FileHandle = NULL;

BgpClearScreen_t					BgpClearScreen = NULL;
BgpGxDrawBitmapImage_t				BgpGxDrawBitmapImage = NULL;

BOOLEAN								IsBiosSystem = FALSE;
GUID								RandomGuid = { 0 };
UNICODE_STRING						RandomGuidString = RTL_CONSTANT_STRING(L"{3098e337-c95e-4802-93a1-a986e8fd20de}");

UNICODE_STRING						RandomFirmwareVarName = RTL_CONSTANT_STRING(L"TransRights");

//===================================================================================//

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObj, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("[%s:%d MSG] KmdfMandelcheck: Initializing driver.\n", __FILE__, __LINE__);

	DriverObj->DriverUnload = DriverUnload;

	NTSTATUS GuidStatus = RtlGUIDFromString(&RandomGuidString, &RandomGuid);
	if(!NT_SUCCESS(GuidStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Error converting string to GUID. Status: 0x%lX\n", __FILE__, __LINE__, GuidStatus);

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	ULONG UnusedLength = 0;
	NTSTATUS FirmwareStatus = ExGetFirmwareEnvironmentVariable(&RandomFirmwareVarName, &RandomGuid, NULL, &UnusedLength, NULL);
	if(FirmwareStatus == STATUS_NOT_IMPLEMENTED)
	{
		DbgPrint("[%s:%d WRN] KmdfMandelcheck: Detected Legacy BIOS system. Using BOOTVID.dll backend...\n", __FILE__, __LINE__);

		IsBiosSystem = TRUE;
	}
	else
	{
		DbgPrint("[%s:%d WRN] KmdfMandelcheck: Detected modern UEFI system. Using Bgp backend...\n", __FILE__, __LINE__);
	}

	NTSTATUS KernelStatus = GetKernelBaseAddress(DriverObj);
	if (!NT_SUCCESS(KernelStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Error retrieving kernel base address. Status: 0x%lX\n", __FILE__, __LINE__, KernelStatus);

		return STATUS_FAILED_DRIVER_ENTRY;
	}

#ifdef _DEBUG
	DbgPrint("[%s:%d DBG] KmdfMandelcheck: Kernel base: 0x%p\n", __FILE__, __LINE__, KernelBaseAddress);
#endif

	//TODO: replace with proper offset detection lmao
	BgpClearScreen = (BgpClearScreen_t)((ULONG_PTR)KernelBaseAddress + 0x65B9B0);
	BgpGxDrawBitmapImage = (BgpGxDrawBitmapImage_t)((ULONG_PTR)KernelBaseAddress + 0xADE730);

#ifdef _DEBUG
	DbgPrint("[%s:%d DBG] KmdfMandelcheck: BgpClearScreen address: 0x%llX\n", __FILE__, __LINE__, (ULONG_PTR)BgpClearScreen);
	DbgPrint("[%s:%d DBG] KmdfMandelcheck: BgpGxDrawBitmapImage address: 0x%llX\n", __FILE__, __LINE__, (ULONG_PTR)BgpGxDrawBitmapImage);
#endif

	NTSTATUS BitmapStatus = ReadBitmapFile();
	if(!NT_SUCCESS(BitmapStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Error reading target bitmap file. Status: 0x%lX\n", __FILE__, __LINE__, BitmapStatus);

		return STATUS_FAILED_DRIVER_ENTRY;
	}

	/*NTSTATUS CallbackStatus = InitializeBugcheckCallback();
	if(!NT_SUCCESS(CallbackStatus))
	{
		DbgPrint("[%s:%d ERR] KmdfMandelcheck: Error initializing bugcheck callback function. Status: 0x%lX\n", __FILE__, __LINE__, CallbackStatus);

		return STATUS_FAILED_DRIVER_ENTRY;
	}*/

	DbgPrint("[%s:%d MSG] KmdfMandelcheck: Driver initialized successfully!\n", __FILE__, __LINE__);

	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObj)
{
	UNREFERENCED_PARAMETER(DriverObj);

	DbgPrint("[%s:%d WRN] KmdfMandelcheck: Unloading driver.\n", __FILE__, __LINE__);

	/*KeDeregisterBugCheckCallback(BugcheckCallbackRecord);
	ExFreePoolWithTag(BugcheckCallbackRecord, 'BCHK');*/

	if(LoadedBitmapFile != NULL)
	{
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');
	}
}

NTSTATUS GetKernelBaseAddress(PDRIVER_OBJECT DriverObj)
{
	PLDR_DATA_TABLE_ENTRY DriverEntry = (PLDR_DATA_TABLE_ENTRY)DriverObj->DriverSection;
	PLDR_DATA_TABLE_ENTRY FirstEntry = DriverEntry;

	while((PLDR_DATA_TABLE_ENTRY)DriverEntry->InLoadOrderLinks.Flink != FirstEntry)
	{
		if(RtlCompareUnicodeString(&DriverEntry->BaseDllName, &KernelFileName, TRUE) == 0)
		{
			KernelBaseAddress = DriverEntry->DllBase;

			return STATUS_SUCCESS;
		}

		DriverEntry = (PLDR_DATA_TABLE_ENTRY)DriverEntry->InLoadOrderLinks.Flink;
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS ReadBitmapFile(VOID)
{
	//Allocate the buffer. Must be nonpaged for it to be accessible inside the bugcheck
	//callback.
	LoadedBitmapFile = ExAllocatePoolZero(NonPagedPoolNx, BMP_BUFF_CAP, 'BTMP');
	if (LoadedBitmapFile == NULL) return STATUS_INSUFFICIENT_RESOURCES;

	OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
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
		ZwClose(FileHandle);
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');

		return FileStatus;
	}

	//Actually perform the read operation into the buffer.
	FileStatus = ZwReadFile(FileHandle, NULL, NULL,
							NULL, &FileStatusBlock, LoadedBitmapFile,
							BMP_BUFF_CAP, NULL, NULL);
	if(!NT_SUCCESS(FileStatus))
	{
		ZwClose(FileHandle);
		ExFreePoolWithTag(LoadedBitmapFile, 'BTMP');

		return FileStatus;
	}

	//Close handle to file. We don't need it anymore.
	ZwClose(FileHandle);

	return STATUS_SUCCESS;
}

NTSTATUS InitializeBugcheckCallback(VOID)
{
	//Allocate memory for the bugcheck callback record.
	//Must be nonpaged for it to work.
	BugcheckCallbackRecord = ExAllocatePoolZero(NonPagedPool, sizeof(KBUGCHECK_CALLBACK_RECORD), 'BCHK');
	if(BugcheckCallbackRecord == NULL) return STATUS_INSUFFICIENT_RESOURCES;

	KeInitializeCallbackRecord(BugcheckCallbackRecord);

	BOOLEAN Registered = KeRegisterBugCheckCallback(BugcheckCallbackRecord, BugcheckCallback, NULL, 0, (PUCHAR)"KmdfMandelcheck");
	if(!Registered)
	{
		ExFreePoolWithTag(BugcheckCallbackRecord, 'BCHK');

		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

VOID BugcheckCallback(PVOID Buffer, ULONG Length)
{
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(Length);

	DbgPrint("[%s:%d MSG] KmdfMandelcheck: Bugcheck callback has been called!\n", __FILE__, __LINE__);

	SCREEN_OFFSET BitmapOffset = { 0, 0 };

	InbvAcquireDisplayOwnership();

	BgpClearScreen(0xFF000000);

	BgpGxDrawBitmapImage(LoadedBitmapFile, BitmapOffset);

	while(TRUE)
	{
		__nop();
	}
}
