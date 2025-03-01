/** @file ngage_application.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef NGAGE_APPLICATION_H
#define NGAGE_APPLICATION_H

#include <aknapp.h>

class CNGageApplication : public CAknApplication
{
public:
    TUid AppDllUid() const;

protected:
    CApaDocument* CreateDocumentL();
};

#endif /* NGAGE_APPLICATION_H */
