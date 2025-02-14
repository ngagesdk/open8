/** @file ngage.cpp
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "ngage_application.h"

GLDEF_C TInt E32Dll(TDllReason /* aReason */)
{
    return KErrNone;
}

EXPORT_C CApaApplication* NewApplication()
{
    return (new CNGageApplication);
}
