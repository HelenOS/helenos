/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    misc.c

Abstract:

    Misc EFI support functions



Revision History

--*/

#include "lib.h"


//
// Additional Known guids
//

#define SHELL_INTERFACE_PROTOCOL \
    { 0x47c7b223, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define ENVIRONMENT_VARIABLE_ID  \
    { 0x47c7b224, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define DEVICE_PATH_MAPPING_ID  \
    { 0x47c7b225, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define PROTOCOL_ID_ID  \
    { 0x47c7b226, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define ALIAS_ID  \
    { 0x47c7b227, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

static EFI_GUID ShellInterfaceProtocol = SHELL_INTERFACE_PROTOCOL;
static EFI_GUID SEnvId                 = ENVIRONMENT_VARIABLE_ID;
static EFI_GUID SMapId                 = DEVICE_PATH_MAPPING_ID;
static EFI_GUID SProtId                = PROTOCOL_ID_ID;
static EFI_GUID SAliasId               = ALIAS_ID;

static struct {
    EFI_GUID        *Guid;
    WCHAR           *GuidName;
} KnownGuids[] = {
	{  &NullGuid,                  L"G0"},
	{  &EfiGlobalVariable,         L"Efi"},

	{  &VariableStoreProtocol,     L"varstore"},
	{  &DevicePathProtocol,        L"dpath"},
	{  &LoadedImageProtocol,       L"image"},
	{  &TextInProtocol,            L"txtin"},
	{  &TextOutProtocol,           L"txtout"},
	{  &BlockIoProtocol,           L"blkio"},
	{  &DiskIoProtocol,            L"diskio"},
	{  &FileSystemProtocol,        L"fs"},
	{  &LoadFileProtocol,          L"load"},
	{  &DeviceIoProtocol,          L"DevIo"},

	{  &GenericFileInfo,           L"GenFileInfo"},
	{  &FileSystemInfo,            L"FileSysInfo"},

	{  &UnicodeCollationProtocol,  L"unicode"},
	{  &LegacyBootProtocol,        L"LegacyBoot"},
	{  &SerialIoProtocol,          L"serialio"},
	{  &VgaClassProtocol,          L"vgaclass"},
	{  &SimpleNetworkProtocol,     L"net"},
	{  &NetworkInterfaceIdentifierProtocol,    L"nii"},
	{  &PxeBaseCodeProtocol,       L"pxebc"},
	{  &PxeCallbackProtocol,       L"pxecb"},

	{  &VariableStoreProtocol,     L"varstore"},
	{  &LegacyBootProtocol,        L"LegacyBoot"},
	{  &VgaClassProtocol,          L"VgaClass"},
	{  &TextOutSpliterProtocol,    L"TxtOutSplit"},
	{  &ErrorOutSpliterProtocol,   L"ErrOutSplit"},
	{  &TextInSpliterProtocol,     L"TxtInSplit"},
	{  &PcAnsiProtocol,            L"PcAnsi"},
	{  &Vt100Protocol,             L"Vt100"},
	{  &UnknownDevice,             L"Unknown Device"},

	{  &EfiPartTypeSystemPartitionGuid,    L"ESP"},
	{  &EfiPartTypeLegacyMbrGuid,          L"GPT MBR"},

	{  &ShellInterfaceProtocol,    L"ShellInt"},
	{  &SEnvId,                    L"SEnv"},
	{  &SProtId,                   L"ShellProtId"},
	{  &SMapId,                    L"ShellDevPathMap"},
	{  &SAliasId,                  L"ShellAlias"},

	{  NULL }
};

//
//
//

LIST_ENTRY          GuidList;


VOID
InitializeGuid (
    VOID
    )
{
}

INTN
CompareGuid(
    IN EFI_GUID     *Guid1,
    IN EFI_GUID     *Guid2
    )
/*++

Routine Description:

    Compares to GUIDs

Arguments:

    Guid1       - guid to compare
    Guid2       - guid to compare

Returns:
    = 0     if Guid1 == Guid2

--*/
{
    return RtCompareGuid (Guid1, Guid2);
}


VOID
GuidToString (
    OUT CHAR16      *Buffer,
    IN EFI_GUID     *Guid
    )
{

    UINTN           Index;

    //
    // Else, (for now) use additional internal function for mapping guids
    //

    for (Index=0; KnownGuids[Index].Guid; Index++) {
        if (CompareGuid(Guid, KnownGuids[Index].Guid) == 0) {
            SPrint (Buffer, 0, KnownGuids[Index].GuidName);
            return ;
        }
    }

    //
    // Else dump it
    //

    SPrint (Buffer, 0, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        Guid->Data1,                    
        Guid->Data2,
        Guid->Data3,
        Guid->Data4[0],
        Guid->Data4[1],
        Guid->Data4[2],
        Guid->Data4[3],
        Guid->Data4[4],
        Guid->Data4[5],
        Guid->Data4[6],
        Guid->Data4[7]
        );
}
