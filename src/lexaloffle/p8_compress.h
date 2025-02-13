/*
    p8_compress.c

    (c) Copyright 2014-2016 Lexaloffle Games LLP
    author: joseph@lexaloffle.com

    compression used in code section of .p8.png format

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/

#ifndef P8_COMPRESS_H
#define P8_COMPRESS_H

typedef unsigned char uint8;

int decompress_mini(uint8* in_p, uint8* out_p, int max_len);
int pxa_decompress(uint8* in_p, uint8* out_p, int max_len);

#endif // P8_COMPRESS_H
