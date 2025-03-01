/** @file auxiliary.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <stdint.h>

void color_lookup(int col, uint8_t* r, uint8_t* g, uint8_t* b)
{
    switch (col)
    {
        case 0: // black
            *r = 0x00;
            *g = 0x00;
            *b = 0x00;
            break;
        case 1: // dark-blue
            *r = 0x1d;
            *g = 0x2b;
            *b = 0x53;
            break;
        case 2: // dark-purple
            *r = 0x7e;
            *g = 0x25;
            *b = 0x53;
            break;
        case 3: // dark-green
            *r = 0x00;
            *g = 0x87;
            *b = 0x51;
            break;
        case 4: // brown
            *r = 0xab;
            *g = 0x52;
            *b = 0x36;
            break;
        case 5: // dark-grey
            *r = 0x5f;
            *g = 0x57;
            *b = 0x4f;
            break;
        case 6: // light-grey
        default:
            *r = 0xc2;
            *g = 0xc3;
            *b = 0xc7;
            break;
        case 7: // white
            *r = 0xff;
            *g = 0xf1;
            *b = 0xe8;
            break;
        case 8: // red
            *r = 0xff;
            *g = 0x00;
            *b = 0x4d;
            break;
        case 9: // orange
            *r = 0xff;
            *g = 0xa3;
            *b = 0x00;
            break;
        case 10: // yellow
            *r = 0xff;
            *g = 0xec;
            *b = 0x27;
            break;
        case 11: // green
            *r = 0x00;
            *g = 0xe4;
            *b = 0x36;
            break;
        case 12: // blue
            *r = 0x29;
            *g = 0xad;
            *b = 0xff;
            break;
        case 13: // lavender
            *r = 0x83;
            *g = 0x76;
            *b = 0x9c;
            break;
        case 14: // pink
            *r = 0xff;
            *g = 0x77;
            *b = 0xa8;
            break;
        case 15: // light-peach
            *r = 0xff;
            *g = 0xcc;
            *b = 0xaa;
            break;
        case -16: // brownish-black
        case 128:
            *r = 0x29;
            *g = 0x18;
            *b = 0x14;
            break;
        case -15: // darker-blue
        case 129:
            *r = 0x11;
            *g = 0x1d;
            *b = 0x35;
            break;
        case -14: // darker-purple
        case 130:
            *r = 0x42;
            *g = 0x21;
            *b = 0x36;
            break;
        case -13: // blue-green
        case 131:
            *r = 0x12;
            *g = 0x53;
            *b = 0x59;
            break;
        case -12: // dark-brown
        case 132:
            *r = 0x74;
            *g = 0x2f;
            *b = 0x29;
            break;
        case -11: // darker-grey
        case 133:
            *r = 0x49;
            *g = 0x33;
            *b = 0x3b;
            break;
        case -10: // medium-grey
        case 134:
            *r = 0xa2;
            *g = 0x88;
            *b = 0x79;
            break;
        case -9: // light-yellow
        case 135:
            *r = 0xf3;
            *g = 0xef;
            *b = 0x7d;
            break;
        case -8: // dark-red
        case 136:
            *r = 0xbe;
            *g = 0x12;
            *b = 0x50;
            break;
        case -7: // dark-orange
        case 137:
            *r = 0xff;
            *g = 0x6c;
            *b = 0x24;
            *b = 0x24;
            break;
        case -6: // lime-green
        case 138:
            *r = 0xa8;
            *g = 0xe7;
            *b = 0x2e;
            break;
        case -5: // medium-green
        case 139:
            *r = 0x00;
            *g = 0xb5;
            *b = 0x43;
            break;
        case -4: // true-blue
        case 140:
            *r = 0x06;
            *g = 0x5a;
            *b = 0xb5;
            break;
        case -3: // mauve
        case 141:
            *r = 0x75;
            *g = 0x46;
            *b = 0x65;
            break;
        case -2: // dark-peach
        case 142:
            *r = 0xff;
            *g = 0x6e;
            *b = 0x59;
            break;
        case -1: // peach
        case 143:
            *r = 0xff;
            *g = 0x9d;
            *b = 0x81;
            break;
    }
}

uint32_t lookup_color(int col)
{
    uint8_t r, g, b;
    color_lookup(col, &r, &g, &b);
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}
