#pragma once

#include <ntddk.h>
#include <intrin.h>
#include "bootvid.h"

//Limit allocated memory for the bitmap to 64 kilobytes.
#define BMP_BUFF_CAP 65536

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _OFFSET
{
	ULONG X;
	ULONG Y;
} SCREEN_OFFSET, * PSCREEN_OFFSET;

NTKERNELAPI VOID InbvAcquireDisplayOwnership(VOID);

typedef NTSTATUS(__stdcall* BgpClearScreen_t) (ULONG RgbColor);
typedef NTSTATUS(__stdcall* BgpGxDrawBitmapImage_t) (PVOID RawBitmap, SCREEN_OFFSET Offset);
