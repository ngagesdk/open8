/** @file ngage_appview.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef NGAGE_APPVIEW_H
#define NGAGE_APPVIEW_H

#include <coecntrl.h>

class CNGageAppView : public CCoeControl
{
public:
    static CNGageAppView* NewL(const TRect& aRect);
    static CNGageAppView* NewLC(const TRect& aRect);

    ~CNGageAppView();

public:
    void Draw(const TRect& aRect) const;

private:
    void ConstructL(const TRect& aRect);

    CNGageAppView();
};

#endif /* NGAGE_APPVIEW_H */
