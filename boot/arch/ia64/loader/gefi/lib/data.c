/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    data.c

Abstract:

    EFI library global data



Revision History

--*/

#include "lib.h"

//
// LibInitialized - TRUE once InitializeLib() is called for the first time
//

BOOLEAN  LibInitialized = FALSE;

//
// ST - pointer to the EFI system table
//

EFI_SYSTEM_TABLE        *ST;

//
// BS - pointer to the boot services table
//

EFI_BOOT_SERVICES       *BS;


//
// Default pool allocation type
//

EFI_MEMORY_TYPE PoolAllocationType = EfiBootServicesData;

//
// Unicode collation functions that are in use
//

EFI_UNICODE_COLLATION_INTERFACE   LibStubUnicodeInterface = {
    LibStubStriCmp,
    LibStubMetaiMatch,
    LibStubStrLwrUpr,
    LibStubStrLwrUpr,
    NULL,   // FatToStr
    NULL,   // StrToFat
    NULL    // SupportedLanguages
}; 

EFI_UNICODE_COLLATION_INTERFACE   *UnicodeInterface = &LibStubUnicodeInterface;

//
// Root device path
//

EFI_DEVICE_PATH RootDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH,0}}
};

EFI_DEVICE_PATH EndDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
};

EFI_DEVICE_PATH EndInstanceDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
};


//
// EFI IDs
//

EFI_GUID EfiGlobalVariable  = EFI_GLOBAL_VARIABLE;
EFI_GUID NullGuid = { 0,0,0,{0,0,0,0,0,0,0,0} };

//
// Protocol IDs
//

EFI_GUID DevicePathProtocol       = DEVICE_PATH_PROTOCOL;
EFI_GUID LoadedImageProtocol      = LOADED_IMAGE_PROTOCOL;
EFI_GUID TextInProtocol           = SIMPLE_TEXT_INPUT_PROTOCOL;
EFI_GUID TextOutProtocol          = SIMPLE_TEXT_OUTPUT_PROTOCOL;
EFI_GUID BlockIoProtocol          = BLOCK_IO_PROTOCOL;
EFI_GUID DiskIoProtocol           = DISK_IO_PROTOCOL;
EFI_GUID FileSystemProtocol       = SIMPLE_FILE_SYSTEM_PROTOCOL;
EFI_GUID LoadFileProtocol         = LOAD_FILE_PROTOCOL;
EFI_GUID DeviceIoProtocol         = DEVICE_IO_PROTOCOL;
EFI_GUID UnicodeCollationProtocol = UNICODE_COLLATION_PROTOCOL;
EFI_GUID SerialIoProtocol         = SERIAL_IO_PROTOCOL;
EFI_GUID SimpleNetworkProtocol    = EFI_SIMPLE_NETWORK_PROTOCOL;
EFI_GUID PxeBaseCodeProtocol      = EFI_PXE_BASE_CODE_PROTOCOL;
EFI_GUID PxeCallbackProtocol      = EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL;
EFI_GUID NetworkInterfaceIdentifierProtocol = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL;
EFI_GUID UiProtocol               = EFI_UI_PROTOCOL;
//
// File system information IDs
//

EFI_GUID GenericFileInfo           = EFI_FILE_INFO_ID;
EFI_GUID FileSystemInfo            = EFI_FILE_SYSTEM_INFO_ID;
EFI_GUID FileSystemVolumeLabelInfo = EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID;

//
// Reference implementation public protocol IDs
//

EFI_GUID InternalShellProtocol = INTERNAL_SHELL_GUID;
EFI_GUID VariableStoreProtocol = VARIABLE_STORE_PROTOCOL;
EFI_GUID LegacyBootProtocol = LEGACY_BOOT_PROTOCOL;
EFI_GUID VgaClassProtocol = VGA_CLASS_DRIVER_PROTOCOL;

EFI_GUID TextOutSpliterProtocol = TEXT_OUT_SPLITER_PROTOCOL;
EFI_GUID ErrorOutSpliterProtocol = ERROR_OUT_SPLITER_PROTOCOL;
EFI_GUID TextInSpliterProtocol = TEXT_IN_SPLITER_PROTOCOL;

EFI_GUID AdapterDebugProtocol = ADAPTER_DEBUG_PROTOCOL;

//
// Device path media protocol IDs
//
EFI_GUID PcAnsiProtocol = DEVICE_PATH_MESSAGING_PC_ANSI;
EFI_GUID Vt100Protocol  = DEVICE_PATH_MESSAGING_VT_100;

//
// EFI GPT Partition Type GUIDs
//
EFI_GUID EfiPartTypeSystemPartitionGuid = EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;
EFI_GUID EfiPartTypeLegacyMbrGuid = EFI_PART_TYPE_LEGACY_MBR_GUID;


//
// Reference implementation Vendor Device Path Guids
//
EFI_GUID UnknownDevice      = UNKNOWN_DEVICE_GUID;

//
// Configuration Table GUIDs
//

EFI_GUID MpsTableGuid             = MPS_TABLE_GUID;
EFI_GUID AcpiTableGuid            = ACPI_TABLE_GUID;
EFI_GUID SMBIOSTableGuid          = SMBIOS_TABLE_GUID;
EFI_GUID SalSystemTableGuid       = SAL_SYSTEM_TABLE_GUID;
