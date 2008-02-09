#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE *image, EFI_SYSTEM_TABLE *systab)
{
	UINTN index;

	systab->ConOut->OutputString(systab->ConOut, L"Hello application started\r\n");
	systab->ConOut->OutputString(systab->ConOut, L"\r\n\r\n\r\nHit any key to exit\r\n");
	systab->BootServices->WaitForEvent(1, &systab->ConIn->WaitForKey, &index);
	return EFI_SUCCESS;
}
