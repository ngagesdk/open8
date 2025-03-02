/** @file core.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include "lexaloffle/p8_compress.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"
#include "api.h"
#include "config.h"
#include "core.h"
#include "image_loader.h"
#include "memory.h"
#include "misc/stb_image.h"

#ifdef _WIN32
#include "misc/dirent.h"
#else
#include <dirent.h>
#endif
#include <string.h>

static SDL_Texture* frame;

static char** available_carts;
static int num_carts;

static cart_t cart;
static state_t state;
static lua_State* vm;

static int prev_selection;
static int selection;

static void* mem_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;

    if (nsize == 0)
    {
        SDL_free(ptr);
        return NULL;
    }
    else
    {
        return SDL_realloc(ptr, nsize);
    }
}

static bool init_vm(SDL_Renderer* renderer)
{
    vm = lua_newstate(mem_allocator, NULL);
    if (!vm)
    {
        SDL_Log("Couldn't create Lua state.");
        return false;
    }
    lua_setpico8memory(vm, pico8_ram);
    luaL_openlibs(vm);
    register_api(vm, renderer);

    puts("\r");
    if (luaL_dostring(vm, "log('Lua VM initialized successfully')"))
    {
        SDL_Log("Lua VM could not be initialised: %s", lua_tostring(vm, -1));
        return false;
    }
    SDL_Log("Lua memory usage: %d bytes",
        lua_gc(vm, LUA_GCCOUNT, 0) * 1024 + lua_gc(vm, LUA_GCCOUNTB, 0));

    return true;
}

static void destroy_vm(void)
{
    lua_close(vm);
}

static void extract_pico8_data(const uint8_t* image_data, uint8_t* cart_data)
{
    size_t data_index = 0;
    size_t pixel_count = CART_WIDTH * CART_HEIGHT;

    // Each Pico-8 byte is stored as the two least significant bits of each of the four
    // channels, ordered ABGR (E.g: the A channel stores the 2 most significant bits in
    // the bytes).  The image is 160 pixels wide and 205 pixels high, for a possible
    // storage of 32,800 (0x8020) bytes.

    for (size_t i = 0; i < pixel_count; i++)
    {
        if (data_index >= CART_DATA_SIZE)
        {
            break;
        }

        // ABGR8888
        uint8_t A = image_data[i * 4];     // A channel
        uint8_t B = image_data[i * 4 + 1]; // B channel
        uint8_t G = image_data[i * 4 + 2]; // G channel
        uint8_t R = image_data[i * 4 + 3]; // R channel

        // Extract the 2 least significant bits from each channel.
        uint8_t byte = ((B & 0x03) << 6) | ((G & 0x03) << 4) | ((R & 0x03) << 2) | (A & 0x03);

        // Swap nibbles.
        byte = (byte >> 4) | (byte << 4);

        cart_data[data_index] = byte;
        data_index++;
    }
}

static int load_cart(SDL_Renderer* renderer, const char* file_name, cart_t* cart)
{
    int width, height, bpp;
    char path[256];

    SDL_snprintf(path, sizeof(path), "%scarts/%s", SDL_GetBasePath(), file_name);

    FILE* file = fopen(path, "rb");
    if (!file)
    {
        SDL_Log("Couldn't open file: %s", path);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    cart->data = (uint8_t*)SDL_calloc(file_size, sizeof(uint8_t));
    if (!cart->data)
    {
        SDL_Log("Couldn't allocate memory for cart data");
        fclose(file);
        return 0;
    }
    fread(cart->data, 1, file_size, file);
    cart->size = file_size;
    fclose(file);

    uint32_t* image_data = (uint32_t*)stbi_load_from_memory(cart->data, cart->size, &width, &height, &bpp, 4);
    if (!image_data)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        return 0;
    }

    if (width != CART_WIDTH || height != CART_HEIGHT)
    {
        SDL_Log("Invalid image size: %dx%d", width, height);
        return 0;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR8888, image_data, width * 4);
    if (!surface)
    {
        SDL_Log("Couldn't create surface: %s", SDL_GetError());
        stbi_image_free(image_data);
        return 0;
    }

    extract_pico8_data((const uint8_t*)image_data, cart->cart_data);

    uint32_t header = *(uint32_t*)&cart->cart_data[0x4300];
    int status = 0;

    cart->code = SDL_calloc(MAX_CODE_SIZE, sizeof(uint8_t));
    if (!cart->code)
    {
        SDL_Log("Could not allocate code memory: %s", SDL_GetError());
        stbi_image_free(image_data);
        return 0;
    }

    if (0x003a633a == header) // :c: followed by \x00
    {
        // Code is compressed (old format).
        status = decompress_mini(&cart->cart_data[0x4300], cart->code, MAX_CODE_SIZE);
        cart->code_size = cart->cart_data[0x4304] << 8 | cart->cart_data[0x4305];
    }
    else if (0x61787000 == header) // \x00 followed by pxa
    {
        // Code is compressed (new format, v0.2.0+).
        status = pxa_decompress(&cart->cart_data[0x4300], cart->code, MAX_CODE_SIZE);
        cart->code_size = cart->cart_data[0x4304] << 8 | cart->cart_data[0x4305];
    }
    else
    {
        for (cart->code_size = 0; cart->code_size < MAX_CODE_SIZE; cart->code_size++)
        {
            if (cart->cart_data[0x4300 + cart->code_size] == 0)
            {
                break;
            }
            else
            {
                cart->code[cart->code_size] = cart->cart_data[0x4300 + cart->code_size];
            }
        }
    }

    // Release the allocated memory we don't need.
    cart->code = SDL_realloc(cart->code, cart->code_size);
    if (!cart->code)
    {
        SDL_Log("Could not re-allocate code memory: %s", SDL_GetError());
        stbi_image_free(image_data);
        return 0;
    }

    if (status == 1)
    {
        cart->is_corrupt = true;
    }
    else
    {
        cart->is_corrupt = false;
    }

    cart->image = SDL_CreateTextureFromSurface(renderer, surface);
    if (!cart->image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return 0;
    }
    SDL_DestroySurface(surface);
    stbi_image_free(image_data);

    if (!SDL_SetTextureScaleMode(cart->image, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    return 1;
}

static void destroy_cart(cart_t* cart)
{
    if (!cart)
    {
        return;
    }

    if (cart->image)
    {
        SDL_DestroyTexture(cart->image);
    }

    if (cart->code)
    {
        SDL_free(cart->code);
    }
    cart->code = NULL;

    if (cart->data)
    {
        SDL_free(cart->data);
    }
    cart->data = NULL;
}

static bool is_function_present(lua_State* L, const char* func_name)
{
    lua_getglobal(L, func_name);
    bool exists = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return exists;
}

static void call_pico8_function(lua_State* L, const char* func_name)
{
    lua_getglobal(L, func_name);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        SDL_Log("Error calling function %s: %s", func_name, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void print_memory_usage(lua_State* L)
{
    SDL_Log("Lua memory usage: %d bytes",
        lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc(L, LUA_GCCOUNTB, 0));
}

static bool run_script(SDL_Renderer* renderer, const char* file_name)
{
    char path[256];
    SDL_snprintf(path, sizeof(path), "%scarts/%s", SDL_GetBasePath(), file_name);

    if (luaL_loadfile(vm, path) || lua_pcall(vm, 0, 0, 0))
    {
        SDL_Log("Could not run .p8 script: %s", lua_tostring(vm, -1));
        print_memory_usage(vm);
        lua_pop(vm, 1);
        return false;
    }

    print_memory_usage(vm);

    if (is_function_present(vm, "_init"))
    {
        call_pico8_function(vm, "_init");
    }
    state = STATE_EMULATOR;
    return true;
}

static bool run_cartridge(SDL_Renderer* renderer)
{
    SDL_memset(pico8_ram, 0x00, RAM_SIZE);

    if (!cart.is_corrupt)
    {
        state = STATE_EMULATOR;

        if (luaL_loadbuffer(vm, (const char*)cart.code, cart.code_size, "cart") || lua_pcall(vm, 0, 0, 0))
        {
            SDL_Log("Could not run cartridge: %s", lua_tostring(vm, -1));
            print_memory_usage(vm);
            lua_pop(vm, 1);
            return false;
        }

        print_memory_usage(vm);

        if (is_function_present(vm, "_init"))
        {
            call_pico8_function(vm, "_init");
        }
    }
    else
    {
        return false;
    }

    return true;
}

static void select_next_cartridge(SDL_Renderer* renderer)
{
    destroy_cart(&cart);
    prev_selection = selection;
    selection++;
    if (selection >= num_carts)
    {
        selection = 0;
    }
    load_cart(renderer, available_carts[selection], &cart);
}

static void select_prev_cartridge(SDL_Renderer* renderer)
{
    destroy_cart(&cart);
    prev_selection = selection;
    selection--;
    if (selection < 0)
    {
        selection = num_carts - 1;
    }
    load_cart(renderer, available_carts[selection], &cart);
}

static void render_cartridge(SDL_Renderer* renderer)
{
    SDL_SetRenderTarget(renderer, NULL);

    SDL_FRect source;
    SDL_FRect dest;

    source.x = 16.f;
    source.y = 24.f;
    source.w = SCREEN_SIZE;
    source.h = SCREEN_SIZE;

    dest.x = SCREEN_OFFSET_X;
    dest.y = SCREEN_OFFSET_Y;
    dest.w = SCREEN_SIZE;
    dest.h = SCREEN_SIZE;

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, frame, NULL, NULL);
    SDL_RenderTexture(renderer, cart.image, &source, &dest);
}

bool init_core(SDL_Renderer* renderer)
{
    char path[256];
    int width, height, bpp;

    num_carts = 0;
    selection = 0;
    prev_selection = !selection;

    SDL_snprintf(path, sizeof(path), "%sdata/frame.png", SDL_GetBasePath());
    frame = load_image(renderer, path, &width, &height, &bpp);
    if (!frame)
    {
        return false;
    }
    if (width != WINDOW_W || height != WINDOW_H)
    {
        SDL_Log("Invalid frame size: %dx%d", width, height);
        SDL_DestroyTexture(frame);
        return false;
    }

    SDL_snprintf(path, sizeof(path), "%scarts", SDL_GetBasePath());

    DIR* dir = opendir(path);
    if (!dir)
    {
        SDL_Log("Couldn't open directory: %s", path);
        return false;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)))
    {
       if (SDL_strstr(entry->d_name, ".p8.png"))
       {
           available_carts = SDL_realloc(available_carts, (num_carts + 1) * sizeof(char*));
           available_carts[num_carts] = SDL_strdup(entry->d_name);
           num_carts++;
       }
    }
    closedir(dir);

    if (!num_carts)
    {
        SDL_Log("No carts found in directory: %s", path);
        return false;
    }
    if (!load_cart(renderer, (const char*)available_carts[0], &cart))
    {
        return false;
    }

    if (!init_vm(renderer))
    {
        return false;
    }

    return init_memory(renderer);
}

void destroy_core(void)
{
    destroy_memory();
    destroy_vm();
    destroy_cart(&cart);
    for (int i = 0; i < num_carts; i++)
    {
        SDL_free(available_carts[i]);
    }
    if (available_carts)
    {
        SDL_free(available_carts);
    }
    if (frame)
    {
        SDL_DestroyTexture(frame);
    }
}

bool handle_events(SDL_Renderer* renderer, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return false;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.repeat) // No key repeat.
            {
                break;
            }
            if (state == STATE_MENU)
            {
                switch (event->key.key)
                {
                    case SDLK_SOFTLEFT:
                    case SDLK_ESCAPE:
                        return false;
                    case SDLK_5:
                    case SDLK_KP_5:
                    case SDLK_SELECT:
                    case SDLK_SPACE:
                        run_cartridge(renderer);
                        return true;
                    case SDLK_7:
                    case SDLK_LALT:
                        SDL_Log("Running test script");
                        run_script(renderer, "api_test.p8");
                        return true;
                    case SDLK_LEFT:
                    case SDLK_A:
                        select_prev_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                    case SDLK_RIGHT:
                    case SDLK_D:
                        select_next_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                    case SDLK_HASH: // Show FPS on the N-Gage.
                        render_cartridge(renderer);
                }
            }
            else if (state == STATE_EMULATOR)
            {
                switch (event->key.key)
                {
                    case SDLK_SOFTLEFT:
                    case SDLK_ESCAPE:
                        destroy_vm();
                        init_vm(renderer);
                        reset_memory();
                        state = STATE_MENU;
                        return true;
                }
            }
            break;
        }
    }
    return true;
}

bool iterate_core(SDL_Renderer* renderer)
{
    if (state == STATE_MENU)
    {
        if (selection != prev_selection)
        {
            render_cartridge(renderer);
            SDL_RenderPresent(renderer);
        }
    }
    else if (state == STATE_EMULATOR)
    {
        if (is_function_present(vm, "_update"))
        {
            call_pico8_function(vm, "_update");
        }
        else if (is_function_present(vm, "_update60"))
        {
            call_pico8_function(vm, "_update60");
        }

        if (is_function_present(vm, "_draw"))
        {
            call_pico8_function(vm, "_draw");
            update_time();
            update_from_virtual_memory(renderer);
        }

        SDL_RenderPresent(renderer);
    }
    SDL_Delay(1);

    return true;
}
