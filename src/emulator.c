/** @file emulator.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdio.h>
#include "lexaloffle/p8_compress.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"
#include "api.h"
#include "config.h"
#include "emulator.h"
#include "image_loader.h"
#include "stb_image.h"

#include <dirent.h>
#include <string.h>

static SDL_Texture* frame;

static char** available_carts;
static int num_carts;

static cart_t cart;
static state_t state;
static lua_State* vm;

static int selection;

static bool init_vm(SDL_Renderer* renderer);
static void destroy_vm(void);
static int load_cart(SDL_Renderer* renderer, const char* file_name, cart_t* cart);
static void destroy_cart(cart_t* cart);
static void extract_pico8_data(const Uint8* image_data, Uint8* cart_data);
static bool is_function_present(lua_State* L, const char* func_name);
static void call_pico8_function(lua_State *L, const char *func_name);

bool init_emulator(SDL_Renderer* renderer)
{
    char path[256];
    int width, height, bpp;

    num_carts = 0;
    selection = 0;

    SDL_snprintf(path, sizeof(path), "%sdata/frame.png", SDL_GetBasePath());
    frame = load_image(renderer, path, &width, &height, &bpp);
    if (!frame)
    {
        return false;
    }
    if (width != NGAGE_W || height != NGAGE_H)
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

    return init_vm(renderer);
}

void destroy_emulator(void)
{
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

void select_next(SDL_Renderer* renderer)
{
    destroy_cart(&cart);
    selection++;
    if (selection >= num_carts)
    {
        selection = 0;
    }
    load_cart(renderer, available_carts[selection], &cart);
}

void select_prev(SDL_Renderer* renderer)
{
    destroy_cart(&cart);
    selection--;
    if (selection < 0)
    {
        selection = num_carts - 1;
    }
    load_cart(renderer, available_carts[selection], &cart);
}

void render_selection(SDL_Renderer* renderer, bool with_frame)
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

    if (with_frame)
    {
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, frame, NULL, NULL);
    }
    SDL_RenderTexture(renderer, cart.image, &source, &dest);
    SDL_RenderPresent(renderer);
}

bool run_selection(SDL_Renderer* renderer)
{
    SDL_memset(pico8_ram, 0x00, RAM_SIZE);

    if (!cart.is_corrupt)
    {
        state = STATE_EMULATOR;

        if (luaL_loadbuffer(vm, (const char*)cart.code, cart.code_size, "cart") || lua_pcall(vm, 0, 0, 0))
        {
            SDL_Log("Lua error: %s", lua_tostring(vm, -1));
            lua_pop(vm, 1);
            return false;
        }
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

#if 0
void run_tests(void)
{
    SDL_Log("Run unit tests.");
    if (luaL_dostring(vm, "dofile('tests.p8')"))
    {
        SDL_Log("Lua error: %s", lua_tostring(vm, -1));
    }
}
#endif

bool handle_event(SDL_Renderer* renderer, SDL_Event* event)
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
                if (event->key.key == SDLK_SOFTLEFT)
                {
                    return false;
                }

                if (event->key.key == SDLK_5 || event->key.key == SDLK_SELECT)
                {
                    run_selection(renderer);
                    return true;
                }

                if (event->key.key == SDLK_LEFT)
                {
                    select_prev(renderer);
                    render_selection(renderer, false);
                    return true;
                }

                if (event->key.key == SDLK_RIGHT)
                {
                    select_next(renderer);
                    render_selection(renderer, false);
                    return true;
                }

                if (event->key.key == SDLK_HASH);
                {
                    render_selection(renderer, true);
                }
            }
            else if (state == STATE_EMULATOR)
            {
                if (event->key.key == SDLK_SOFTLEFT)
                {
                    destroy_vm();
                    init_vm(renderer);
                    state = STATE_MENU;
                    render_selection(renderer, true);
                    return true;
                }
            }
            break;
        }
    }

    return true;
}

bool iterate_emulator(SDL_Renderer* renderer)
{
    if (state == STATE_MENU)
    {
        render_selection(renderer, false);
    }
    else if (state == STATE_EMULATOR)
    {
        if (is_function_present(vm, "_update"))
        {
            call_pico8_function(vm, "_update");
            update_time();
        }
        else if (is_function_present(vm, "_update60"))
        {
            call_pico8_function(vm, "_update60");
            update_time();
        }

        if (is_function_present(vm, "_draw"))
        {
            call_pico8_function(vm, "_draw");
        }
        SDL_RenderPresent(renderer);
    }

    return true;
}

static bool init_vm(SDL_Renderer* renderer)
{
    vm = luaL_newstate();
    if (!vm)
    {
        SDL_Log("Couldn't create Lua state.");
        return false;
    }
    lua_setpico8memory(vm, pico8_ram);
    luaL_openlibs(vm);
    register_api(vm, renderer);

    if (luaL_dostring(vm, "log('Lua VM initialized successfully')"))
    {
        SDL_Log("Lua error: %s", lua_tostring(vm, -1));
        return false;
    }
    return true;
}

static void destroy_vm(void)
{
    lua_close(vm);
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

    cart->data = (Uint8*)SDL_calloc(file_size, sizeof(Uint8));
    if (!cart->data)
    {
        SDL_Log("Couldn't allocate memory for cart data");
        fclose(file);
        return 0;
    }
    fread(cart->data, 1, file_size, file);
    cart->size = file_size;
    fclose(file);

    Uint32* image_data = (Uint32*)stbi_load_from_memory(cart->data, cart->size, &width, &height, &bpp, 4);
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

    extract_pico8_data((const Uint8*)image_data, cart->cart_data);

    Uint32 header = *(Uint32*)&cart->cart_data[0x4300];
    int status = 0;

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

    if (status == 1)
    {
        cart->is_corrupt = true;
    }
    else
    {
        cart->is_corrupt = false;
    }

    stbi_image_free(image_data);

    cart->image = SDL_CreateTextureFromSurface(renderer, surface);
    if (!cart->image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return 0;
    }
    SDL_DestroySurface(surface);

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

    if (cart->data)
    {
        SDL_free(cart->data);
    }
}

static void extract_pico8_data(const Uint8* image_data, Uint8* cart_data)
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
        Uint8 A = image_data[i * 4];     // A channel
        Uint8 B = image_data[i * 4 + 1]; // B channel
        Uint8 G = image_data[i * 4 + 2]; // G channel
        Uint8 R = image_data[i * 4 + 3]; // R channel

        // Extract the 2 least significant bits from each channel.
        Uint8 byte = ((B & 0x03) << 6) | ((G & 0x03) << 4) | ((R & 0x03) << 2) | (A & 0x03);

        // Swap nibbles.
        byte = (byte >> 4) | (byte << 4);

        cart_data[data_index] = byte;
        data_index++;
    }
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
