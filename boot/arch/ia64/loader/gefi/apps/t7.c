#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_INPUT_KEY efi_input_key;
	EFI_STATUS efi_status;

	InitializeLib(image, systab);

	Print(L"HelloLib application started\n");

	Print(L"\n\n\nHit any key to exit this image\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);

	ST->ConOut->OutputString(ST->ConOut, L"\n\n");

	efi_status = ST->ConIn->ReadKeyStroke(ST->ConIn, &efi_input_key);

	Print(L"ScanCode: %xh  UnicodeChar: %xh\n",
		efi_input_key.ScanCode, efi_input_key.UnicodeChar);

	return EFI_SUCCESS;
}
