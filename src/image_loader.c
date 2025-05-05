/** @file image_loader.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include "image_loader.h"

#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

SDL_Texture* load_image(SDL_Renderer* renderer, const char* file_name, int* width, int* height, int* bpp)
{
    FILE* file = fopen(file_name, "rb");
    if (!file)
    {
        SDL_Log("Couldn't open file: %s", file_name);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = (uint8_t*)SDL_calloc(file_size, sizeof(uint8_t));
    if (!data)
    {
        SDL_Log("Couldn't allocate memory for image file data");
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, file_size, file) != file_size)  {
        SDL_Log("Couldn't read memory for image file data");
        fclose(file);
        return NULL;
    }
    fclose(file);

    uint32_t* image_data = (uint32_t*)stbi_load_from_memory(data, file_size, width, height, bpp, 4);
    if (!image_data)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        return NULL;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(*width, *height, SDL_PIXELFORMAT_RGBA32, image_data, *width * 4);
    if (!surface)
    {
        SDL_Log("Couldn't create surface: %s", SDL_GetError());
        stbi_image_free(image_data);
        return NULL;
    }

    SDL_Texture* image = SDL_CreateTextureFromSurface(renderer, surface);
    if (!image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return NULL;
    }
    SDL_DestroySurface(surface);
    stbi_image_free(image_data);

    if (!SDL_SetTextureScaleMode(image, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    return image;
}
