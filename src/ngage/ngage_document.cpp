/** @file ngage_document.cpp
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_appui.h"
#include "ngage_document.h"

CNGageDocument* CNGageDocument::NewL(CEikApplication& aApp)
{
    CNGageDocument* self = NewLC(aApp);
    CleanupStack::Pop(self);
    return self;
}

CNGageDocument* CNGageDocument::NewLC(CEikApplication& aApp)
{
    CNGageDocument* self = new (ELeave) CNGageDocument(aApp);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

void CNGageDocument::ConstructL()
{
    /* No implementation required. */
}

CNGageDocument::CNGageDocument(CEikApplication& aApp) : CAknDocument(aApp)
{
    /* No implementation required. */
}

CNGageDocument::~CNGageDocument()
{
    /* No implementation required. */
}

CEikAppUi* CNGageDocument::CreateAppUiL()
{
    CEikAppUi* appUi = new (ELeave) CNGageAppUi;
    return appUi;
}
