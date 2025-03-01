/** @file ngage_application.cpp
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_document.h"
#include "ngage_application.h"

static const TUid KUidNGageApp = { UID3 };

CApaDocument* CNGageApplication::CreateDocumentL()
{
    CApaDocument* document = CNGageDocument::NewL(*this);
    return document;
}

TUid CNGageApplication::AppDllUid() const
{
    return KUidNGageApp;
}
