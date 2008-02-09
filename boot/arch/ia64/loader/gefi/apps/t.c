#define EFI_SUCCESS		0

typedef short			CHAR16;
typedef unsigned long		UINTN;
typedef unsigned long long	UINT64;
typedef unsigned int		UINT32;
typedef void *			EFI_HANDLE;
typedef UINTN			EFI_STATUS;

typedef struct _EFI_TABLE_HEARDER {
    UINT64                      Signature;
    UINT32                      Revision;
    UINT32                      HeaderSize;
    UINT32                      CRC32;
    UINT32                      Reserved;
} EFI_TABLE_HEADER;

typedef EFI_STATUS (*EFI_TEXT_STRING) (void *This, CHAR16 *String);

typedef struct _SIMPLE_TEXT_OUTPUT_INTERFACE {
    void *                  Reset;

    EFI_TEXT_STRING                 OutputString;
} SIMPLE_TEXT_OUTPUT_INTERFACE;

typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER                Hdr;

    CHAR16                          *FirmwareVendor;
    UINT32                          FirmwareRevision;

    EFI_HANDLE                      ConsoleInHandle;
 /*SIMPLE_INPUT_INTERFACE*/ void    *ConIn;

    EFI_HANDLE                      ConsoleOutHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;

    EFI_HANDLE                      StandardErrorHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *StdErr;

 /*EFI_RUNTIME_SERVICES*/ void            *RuntimeServices;
 /*EFI_BOOT_SERVICES*/ void               *BootServices;

    UINTN                           NumberOfTableEntries;
 /*EFI_CONFIGURATION_TABLE*/void         *ConfigurationTable;

} EFI_SYSTEM_TABLE;

static CHAR16 *
a2u (char *str)
{
	static CHAR16 mem[2048];
	int i;

	for (i = 0; str[i]; ++i)
		mem[i] = (CHAR16) str[i];
	mem[i] = 0;
	return mem;
}

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *conout;

	conout = systab->ConOut;
	conout->OutputString(conout, (CHAR16 *)L"Hello World!\n\r");
	conout->OutputString(conout, a2u("Hello World!\n\r"));

	return EFI_SUCCESS;
}
