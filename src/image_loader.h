/** @file image_loader.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <SDL3/SDL.h>

SDL_Texture* load_image(SDL_Renderer* renderer, const char* file_name, int* width, int* height, int* bpp);

#endif // IMAGE_LOADER_H
