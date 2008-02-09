/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    lib.h

Abstract:

    EFI library header files



Revision History

--*/


#include "efi.h"
#include "efilib.h"
#include "efirtlib.h"

//
// Include non architectural protocols
//
#include "efivar.h"
#include "legacyboot.h"
#include "intload.h"
#include "vgaclass.h"
#include "eficonsplit.h"
#include "adapterdebug.h"
#include "intload.h"

#include "efigpt.h"
#include "libsmbios.h"

//
// Prototypes
//

VOID
InitializeGuid (
    VOID
    );

INTN
LibStubStriCmp (
    IN EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                           *S1,
    IN CHAR16                           *S2
    );

BOOLEAN
LibStubMetaiMatch (
    IN EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                           *String,
    IN CHAR16                           *Pattern
    );

VOID
LibStubStrLwrUpr (
    IN EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                           *Str
    );

BOOLEAN
LibMatchDevicePaths (
    IN  EFI_DEVICE_PATH *Multi,
    IN  EFI_DEVICE_PATH *Single
    );

EFI_DEVICE_PATH *
LibDuplicateDevicePathInstance (
    IN EFI_DEVICE_PATH  *DevPath
    );


//
// Globals
//
extern BOOLEAN                          LibInitialized;
extern BOOLEAN                          LibFwInstance;
extern SIMPLE_TEXT_OUTPUT_INTERFACE     *LibRuntimeDebugOut;
extern EFI_UNICODE_COLLATION_INTERFACE  *UnicodeInterface;
extern EFI_UNICODE_COLLATION_INTERFACE  LibStubUnicodeInterface;
extern EFI_RAISE_TPL                    LibRuntimeRaiseTPL;
extern EFI_RESTORE_TPL                  LibRuntimeRestoreTPL;
