/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    event.c

Abstract:




Revision History

--*/

#include "lib.h"


EFI_EVENT
LibCreateProtocolNotifyEvent (
    IN EFI_GUID             *ProtocolGuid,
    IN EFI_TPL              NotifyTpl,
    IN EFI_EVENT_NOTIFY     NotifyFunction,
    IN VOID                 *NotifyContext,
    OUT VOID                *Registration
    )
{
    EFI_STATUS              Status;
    EFI_EVENT               Event;

    //
    // Create the event
    //

    Status = BS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    NotifyTpl,
                    NotifyFunction,
                    NotifyContext,
                    &Event
                    );
    ASSERT (!EFI_ERROR(Status));

    //
    // Register for protocol notifactions on this event
    //

    Status = BS->RegisterProtocolNotify (
                    ProtocolGuid, 
                    Event, 
                    Registration
                    );

    ASSERT (!EFI_ERROR(Status));

    //
    // Kick the event so we will perform an initial pass of
    // current installed drivers
    //

    BS->SignalEvent (Event);
    return Event;
}


EFI_STATUS
WaitForSingleEvent (
    IN EFI_EVENT        Event,
    IN UINT64           Timeout OPTIONAL
    )
{
    EFI_STATUS          Status;
    UINTN               Index;
    EFI_EVENT           TimerEvent;
    EFI_EVENT           WaitList[2];

    if (Timeout) {
        //
        // Create a timer event
        //

        Status = BS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
        if (!EFI_ERROR(Status)) {

            //
            // Set the timer event
            //

            BS->SetTimer (TimerEvent, TimerRelative, Timeout);
            
            //
            // Wait for the original event or the timer
            //

            WaitList[0] = Event;
            WaitList[1] = TimerEvent;
            Status = BS->WaitForEvent (2, WaitList, &Index);
            BS->CloseEvent (TimerEvent);

            //
            // If the timer expired, change the return to timed out
            //

            if (!EFI_ERROR(Status)  &&  Index == 1) {
                Status = EFI_TIMEOUT;
            }
        }

    } else {

        //
        // No timeout... just wait on the event
        //

        Status = BS->WaitForEvent (1, &Event, &Index);
        ASSERT (!EFI_ERROR(Status));
        ASSERT (Index == 0);
    }

    return Status;
}

VOID
WaitForEventWithTimeout (
    IN  EFI_EVENT       Event,
    IN  UINTN           Timeout,
    IN  UINTN           Row,
    IN  UINTN           Column,
    IN  CHAR16          *String,
    IN  EFI_INPUT_KEY   TimeoutKey,
    OUT EFI_INPUT_KEY   *Key
    )
{
    EFI_STATUS      Status;

    do {
        PrintAt (Column, Row, String, Timeout);
        Status = WaitForSingleEvent (Event, 10000000);
        if (Status == EFI_SUCCESS) {
            if (!EFI_ERROR(ST->ConIn->ReadKeyStroke (ST->ConIn, Key))) {
                return;
            }
        }
    } while (Timeout > 0);
    *Key = TimeoutKey;
}

