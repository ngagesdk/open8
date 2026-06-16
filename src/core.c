/** @file core.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lexaloffle/p8_compress.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"
#include "api.h"
#include "app.h"
#include "core.h"
#include "memory.h"

#define STBI_ONLY_PNG
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

static char** available_carts;
static int num_carts;

static cart_t cart;
static state_t state;
static lua_State* vm;

static int prev_selection;
static int selection;

static bool has_update;
static bool has_update60;
static bool has_draw;

static SDL_Texture* overlay;

SDL_FRect cart_rect;

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

static void update_touch_input(SDL_Renderer* renderer)
{
    // Poll all active fingers for simultaneous multi-touch input in emulator mode.
    if (state != STATE_EMULATOR || screen_rect.w <= 0)
    {
        return;
    }

    int num_fingers = 0;
    SDL_Finger** fingers = SDL_GetTouchFingers(0, &num_fingers);

    if (!fingers || !renderer)
    {
        return;
    }

    // If no fingers detected, keep the state as-is (let events handle clearing).
    if (num_fingers <= 0)
    {
        return;
    }

    // Get drawable size (physical pixels) for high-DPI displays.
    int drawable_w, drawable_h;
    SDL_GetRenderOutputSize(renderer, &drawable_w, &drawable_h);

    if (drawable_w <= 0 || drawable_h <= 0)
    {
        return;
    }

    // Reset button state and accumulate from all active fingers.
    touch_button_state = 0;

    // Get touch regions for button detection.
    int touch_count = 0;
    const touch_region* regions = get_touch_regions(&touch_count);

    // Calculate scale based on screen_rect.
    int scale = (int)(screen_rect.w / 128.0f);
    if (scale < 1) scale = 1;

    // Check all active fingers.
    for (int i = 0; i < num_fingers; i++)
    {
        if (!fingers[i]) continue;

        // Convert normalized touch coordinates (0.0-1.0) to physical pixel coordinates.
        float touch_x = fingers[i]->x * drawable_w;
        float touch_y = fingers[i]->y * drawable_h;

        // Translate to coordinates relative to the game rendering area (screen_rect).
        float local_x = touch_x - screen_rect.x;
        float local_y = touch_y - screen_rect.y;

        // Scale back to virtual 128x128 game coordinate system.
        float virt_x = local_x / scale;
        float virt_y = local_y / scale;

        // Map to the touch regions (which are in the 160x205 space)
        // but offset to the game area (which starts at 16, 24 in that space).
        float region_x = virt_x + 16.0f;  // Game area X offset
        float region_y = virt_y + 24.0f;  // Game area Y offset

        // Check against touch regions and accumulate button bits.
        for (int r = 0; r < touch_count; r++)
        {
            if (point_in_touch_region(region_x, region_y, &regions[r]))
            {
                touch_button_state |= (1 << regions[r].bit);
            }
        }
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
    init_api(vm);

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
    if (vm)
    {
        lua_close(vm);
        vm = NULL;
    }
}

static void extract_pico8_data(uint8_t* image_data, uint8_t* cart_data)
{
    size_t data_index = 0;
    size_t pixel_count = CART_WIDTH * CART_HEIGHT;

    // Each Pico-8 byte is stored as the two least significant bits of each of the four
    // channels, ordered ARGB (E.g: the A channel stores the 2 most significant bits in
    // the bytes).  The image is 160 pixels wide and 205 pixels high, for a possible
    // storage of 32,800 (0x8020) bytes.

    for (size_t i = 0; i < pixel_count; i++)
    {
        if (data_index >= CART_DATA_SIZE)
        {
            break;
        }

        // stb_image stores components in RGBA32 order
        uint8_t R = image_data[i * 4];     // R channel
        uint8_t G = image_data[i * 4 + 1]; // G channel
        uint8_t B = image_data[i * 4 + 2]; // B channel
        uint8_t A = image_data[i * 4 + 3]; // A channel

        // Extract the 2 least significant bits from each channel.
        uint8_t byte = ((A & 0x03) << 6) | ((R & 0x03) << 4) | ((G & 0x03) << 2) | ((B & 0x03) << 0);

        // Clean up graphical artifacts visible on 32bpp displays
        image_data[i * 4] = (R & 0xFC) | (R >> 6);
        image_data[i * 4 + 1] = (G & 0xFC) | (G >> 6);
        image_data[i * 4 + 2] = (B & 0xFC) | (B >> 6);
        image_data[i * 4 + 3] = (A & 0xFC) | (A >> 6);

        cart_data[data_index] = byte;
        data_index++;
    }
}

// It's in fact easier to patch the code than parsing the preset
// fill-pattern characters using the Lua C-API.
static void patch_cart_code(cart_t* cart)
{
    // State machine: 0=normal, 1=double-quote string, 2=single-quote string,
    //                3=line comment, 4=long string [[...]].
    // Special bytes are only wrapped when in normal code (state 0).
    size_t count = 0;
    int state = 0;

    // Pass 1: count special bytes that appear outside string literals and comments.
    for (size_t i = 0; i < cart->code_size; i++)
    {
        unsigned char c = (unsigned char)cart->code[i];

        if (state == 1) // Double-quoted string.
        {
            if (c == '\\')
            {
                i++; // Skip escaped character.
            }
            else if (c == '"')
            {
                state = 0;
            }
        }
        else if (state == 2) // Single-quoted string.
        {
            if (c == '\\')
            {
                i++; // Skip escaped character.
            }
            else if (c == '\'')
            {
                state = 0;
            }
        }
        else if (state == 3) // Line comment.
        {
            if (c == '\n')
            {
                state = 0;
            }
        }
        else if (state == 4) // Long string.
        {
            if (c == ']' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == ']')
            {
                state = 0;
                i++;
            }
        }
        else // Normal code.
        {
            if (c == '"')
            {
                state = 1;
            }
            else if (c == '\'')
            {
                state = 2;
            }
            else if (c == '-' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == '-')
            {
                i++;
                if (i + 2 < cart->code_size && (unsigned char)cart->code[i + 1] == '[' && (unsigned char)cart->code[i + 2] == '[')
                {
                    state = 4;
                    i += 2;
                }
                else
                {
                    state = 3;
                }
            }
            else if (c == '[' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == '[')
            {
                state = 4;
                i++;
            }
            else if ((c >= 128 && c <= 135)
                || c == 139  // Left key.
                || c == 142  // O key.
                || c == 145  // Right key.
                || c == 148  // Up key.
                || c == 151) // X key.
            {
                count++;
            }
        }
    }

    if (count == 0)
    {
        return; // No changes needed.
    }

    // Compute new size with added quotes.
    size_t new_size = cart->code_size + count * 2;
    uint8_t* new_code = (uint8_t*)SDL_malloc(new_size + 1); // +1 for null terminator.
    if (!new_code)
    {
        SDL_Log("Memory reallocation failed!");
        return;
    }

    // Pass 2: copy bytes into new_code, wrapping special bytes outside string literals.
    state = 0;
    size_t j = 0;
    for (size_t i = 0; i < cart->code_size; i++)
    {
        unsigned char c = (unsigned char)cart->code[i];

        if (state == 1) // Double-quoted string.
        {
            new_code[j++] = c;
            if (c == '\\' && i + 1 < cart->code_size)
                new_code[j++] = (unsigned char)cart->code[++i]; // Copy escaped character.
            else if (c == '"')
                state = 0;
        }
        else if (state == 2) // Single-quoted string.
        {
            new_code[j++] = c;
            if (c == '\\' && i + 1 < cart->code_size)
            {
                new_code[j++] = (unsigned char)cart->code[++i]; // Copy escaped character.
            }
            else if (c == '\'')
            {
                state = 0;
            }
        }
        else if (state == 3) // Line comment.
        {
            new_code[j++] = c;
            if (c == '\n')
            {
                state = 0;
            }
        }
        else if (state == 4) // Long string.
        {
            new_code[j++] = c;
            if (c == ']' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == ']')
            {
                new_code[j++] = (unsigned char)cart->code[++i];
                state = 0;
            }
        }
        else // Normal code.
        {
            if (c == '"') {
                state = 1; new_code[j++] = c;
            }
            else if (c == '\'') {
                state = 2; new_code[j++] = c;
            }
            else if (c == '-' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == '-')
            {
                new_code[j++] = c;
                new_code[j++] = (unsigned char)cart->code[++i];
                if (i + 2 < cart->code_size && (unsigned char)cart->code[i + 1] == '[' && (unsigned char)cart->code[i + 2] == '[')
                {
                    new_code[j++] = (unsigned char)cart->code[++i];
                    new_code[j++] = (unsigned char)cart->code[++i];
                    state = 4;
                }
                else state = 3;
            }
            else if (c == '[' && i + 1 < cart->code_size && (unsigned char)cart->code[i + 1] == '[')
            {
                state = 4;
                new_code[j++] = c;
                new_code[j++] = (unsigned char)cart->code[++i];
            }
            else if ((c >= 128 && c <= 135)
                || c == 139  // Left key.
                || c == 142  // O key.
                || c == 145  // Right key.
                || c == 148  // Up key.
                || c == 151) // X key.
            {
                new_code[j++] = '"';
                new_code[j++] = c;
                new_code[j++] = '"';
            }
            else
            {
                new_code[j++] = c;
            }
        }
    }
    new_code[j] = '\0';

    SDL_free(cart->code);
    cart->code = new_code;
    cart->code_size = new_size;
}

static int load_cart(SDL_Renderer* renderer, const char* file_name, cart_t* cart)
{
    int width, height, bpp;

    FILE* file = fopen(file_name, "rb");
    if (!file)
    {
        SDL_Log("Couldn't open file: %s", file_name);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = (uint8_t*)SDL_calloc(file_size, sizeof(uint8_t));
    if (!data)
    {
        SDL_Log("Couldn't allocate memory for cart data");
        fclose(file);
        return 0;
    }
    fread(data, 1, file_size, file);
    fclose(file);

    uint8_t* image_data = stbi_load_from_memory(data, file_size, &width, &height, &bpp, 4);
    if (!image_data)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(data);
        return 0;
    }
    SDL_free(data);

    if (width != CART_WIDTH || height != CART_HEIGHT)
    {
        SDL_Log("Invalid image size: %dx%d", width, height);
        stbi_image_free(image_data);
        return 0;
    }

    extract_pico8_data(image_data, cart->cart_data);

    cart->image = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!cart->image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        stbi_image_free(image_data);
        return 0;
    }

    if (!SDL_SetTextureBlendMode(cart->image, SDL_BLENDMODE_BLEND))
    {
        SDL_Log("Couldn't set texture blend mode: %s", SDL_GetError());
    }

    if (!SDL_UpdateTexture(cart->image, NULL, image_data, width * 4))
    {
        SDL_Log("Couldn't update texture: %s", SDL_GetError());
        SDL_DestroyTexture(cart->image);
        stbi_image_free(image_data);
        return 0;
    }

    stbi_image_free(image_data);

    if (!SDL_SetTextureScaleMode(cart->image, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    uint32_t header = *(uint32_t*)&cart->cart_data[0x4300];
    int status = 0;

    cart->code = SDL_calloc(MAX_CODE_SIZE, sizeof(uint8_t));
    if (!cart->code)
    {
        SDL_Log("Could not allocate code memory: %s", SDL_GetError());
        SDL_DestroyTexture(cart->image);
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
        SDL_DestroyTexture(cart->image);
        return 0;
    }

    if (status == 1)
    {
        cart->is_corrupt = true;
    }
    else
    {
        cart->is_corrupt = false;
        patch_cart_code(cart);
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
    if (luaL_loadfile(vm, file_name) || lua_pcall(vm, 0, 0, 0))
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
    has_update = is_function_present(vm, "_update");
    has_update60 = is_function_present(vm, "_update60");
    has_draw = is_function_present(vm, "_draw");
    state = STATE_EMULATOR;
    return true;
}

static bool run_cartridge(SDL_Renderer* renderer)
{
    destroy_vm();
    if (!init_vm(renderer))
    {
        return false;
    }

    reset_memory();

    // Copy spritesheet, map, flags, music and sound effects data to memory.
    // 0x0000-0x42ff
    SDL_memcpy(pico8_ram, cart.cart_data, 0x42ff * sizeof(uint8_t));

    if (!cart.is_corrupt)
    {
        state = STATE_EMULATOR;

        /* Try to load the (possibly patched) code buffer first. If that fails,
           fall back to the original raw code bytes embedded in the cart data
           (starting at 0x4300). Some carts rely on subtle encoding/escaping that
           can be altered by our patcher; attempt the original on failure so
           carts that work on real PICO-8 still run. */
        if (luaL_loadbuffer(vm, (const char*)cart.code, cart.code_size, "cart") || lua_pcall(vm, 0, 0, 0))
        {
            /* Preserve the error message from the failed attempt. */
            const char* err = lua_tostring(vm, -1);
            print_memory_usage(vm);

            /* Try loading original code bytes from cart.cart_data (offset 0x4300)
               up to the first null terminator. */
            size_t orig_size = 0;
            while (orig_size < MAX_CODE_SIZE && cart.cart_data[0x4300 + orig_size] != 0)
                orig_size++;

            if (orig_size > 0)
            {
                if (luaL_loadbuffer(vm, (const char*)&cart.cart_data[0x4300], orig_size, "cart") || lua_pcall(vm, 0, 0, 0))
                {
                    SDL_Log("Could not run cartridge (patched or original): %s", lua_tostring(vm, -1));
                    lua_pop(vm, 1);
                    return false;
                }
                else
                {
                    SDL_Log("Loaded cartridge using original embedded code after patched load failed: %s", err ? err : "(no message)");
                }
            }
            else
            {
                SDL_Log("Could not run cartridge: %s", err ? err : "(no message)");
                lua_pop(vm, 1);
                return false;
            }
            lua_pop(vm, 1); /* pop original error message */
        }

        print_memory_usage(vm);

        if (is_function_present(vm, "_init"))
        {
            call_pico8_function(vm, "_init");
        }
        has_update = is_function_present(vm, "_update");
        has_update60 = is_function_present(vm, "_update60");
        has_draw = is_function_present(vm, "_draw");
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
    SDL_Window* window = SDL_GetRenderWindow(renderer);
    int w, h;

    SDL_SetRenderDrawColor(renderer, 0x31, 0x31, 0x31, 0xff);
    SDL_RenderClear(renderer);

    if (!SDL_GetWindowSize(window, &w, &h))
    {
        SDL_Log("Unable to get window size: %s", SDL_GetError());
    }

    SDL_FRect dest;

    dest.x = 0;
    dest.y = 0;
    dest.w = cart_rect.w;
    dest.h = cart_rect.h;

    SDL_RenderTexture(renderer, cart.image, &dest, &cart_rect);
    if (overlay)
    {
        SDL_RenderTexture(renderer, overlay, NULL, &cart_rect);
    }
}

void handle_resize(SDL_Renderer* renderer) {
    int native_w, native_h;
    SDL_GetRenderOutputSize(renderer, &native_w, &native_h);

    int base_w = 160;
    int base_h = 205;

    int scale_x = native_w / base_w;
    int scale_y = native_h / base_h;

    int scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale < 1)
    {
        scale = 1;
    }

    int window_w = base_w * scale;
    int window_h = base_h * scale;

    cart_rect.x = ((float)(native_w - window_w) + 0.5f) / 2.f;
    cart_rect.y = ((float)(native_h - window_h) + 0.5f) / 2.f;
    cart_rect.w = (float)window_w;
    cart_rect.h = (float)window_h;

    screen_rect.x = cart_rect.x + (scale * 16);
    screen_rect.y = cart_rect.y + (scale * 24);
    screen_rect.w = (float)(scale * 128);
    screen_rect.h = (float)(scale * 128);

#ifdef __DJGPP__
    screen_rect.y += 1;
#endif
}

static SDL_EnumerationResult dir_callback(void* userdata, const char* dirname, const char* fname)
{
    if (SDL_strstr(fname, ".PNG") || (SDL_strstr(fname, ".png")))
    {
        available_carts = SDL_realloc(available_carts, (num_carts + 1) * sizeof(char*));
        SDL_asprintf(&available_carts[num_carts], "%s%s", dirname, fname);
        num_carts++;
    }

    return SDL_ENUM_CONTINUE;
}

static int load_overlay(SDL_Renderer* renderer)
{
    int width, height, bpp;

    FILE* file = fopen("overlay.png", "rb");
    if (!file)
    {
        SDL_Log("Couldn't open file: overlay.png");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = (uint8_t*)SDL_calloc(file_size, sizeof(uint8_t));
    if (!data)
    {
        SDL_Log("Couldn't allocate memory for overlay data");
        fclose(file);
        return 0;
    }
    fread(data, 1, file_size, file);
    fclose(file);

    uint8_t* image_data = stbi_load_from_memory(data, file_size, &width, &height, &bpp, 4);
    if (!image_data)
    {
        SDL_Log("Couldn't load image data: %s", stbi_failure_reason());
        SDL_free(data);
        return 0;
    }
    SDL_free(data);

    overlay = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!overlay)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        stbi_image_free(image_data);
        return 0;
    }

    if (!SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND))
    {
        SDL_Log("Couldn't set texture blend mode: %s", SDL_GetError());
    }

    if (!SDL_UpdateTexture(overlay, NULL, image_data, width * 4))
    {
        SDL_Log("Couldn't update texture: %s", SDL_GetError());
        SDL_DestroyTexture(overlay);
        stbi_image_free(image_data);
        return 0;
    }

    stbi_image_free(image_data);

    if (!SDL_SetTextureScaleMode(overlay, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    return 1;
}

static void destroy_overlay()
{
    if (overlay)
    {
        SDL_DestroyTexture(overlay);
    }
}

bool init_core(SDL_Renderer* renderer)
{
    char* path;

    num_carts = 0;
    selection = 0;
    prev_selection = !selection;

    if (SDL_asprintf(&path, "%scarts", SDL_GetBasePath()) < 0)
    {
        SDL_Log("Out of memory");
        return false;
    }

    if (!SDL_EnumerateDirectory(path, dir_callback, NULL))
    {
        SDL_Log("Couldn't open directory: %s", path);
        SDL_free(path);
        return false;
    }
    SDL_free(path);

    if (!num_carts)
    {
        SDL_Log("No carts found in directory: %s", path);
        return false;
    }

    if (!load_overlay(renderer))
    {
        SDL_Log("Failed to load overlay.");
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
    destroy_overlay();
}

bool handle_events(SDL_Renderer* renderer, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return false;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        {
            handle_resize(renderer);
            return true;
        }
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            const SDL_JoystickID which = event->gdevice.which;
            SDL_Gamepad* gamepad = SDL_OpenGamepad(which);
            if (!gamepad)
            {
                SDL_Log("Joystick #%" SDL_PRIu32 " could not be opened: %s", which, SDL_GetError());
            }
            else
            {
                SDL_Log("Joystick #%" SDL_PRIu32 " connected: %s", which, SDL_GetGamepadName(gamepad));
            }
            return true;
        }
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            const SDL_JoystickID which = event->gdevice.which;
            SDL_Gamepad* gamepad = SDL_GetGamepadFromID(which);
            if (gamepad)
            {
                SDL_CloseGamepad(gamepad);  /* the joystick was unplugged. */
            }
            return true;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            if (state == STATE_MENU)
            {
                if (event->key.repeat) // No key repeat.
                {
                    break;
                }
                switch (event->key.key)
                {
#ifndef __EMSCRIPTEN__
                    case SDLK_SOFTLEFT:
                    case SDLK_ESCAPE:
                        return false;
#endif
#ifdef __SYMBIAN32__
                    case SDLK_5:
                    case SDLK_KP_5:
#endif
                    case SDLK_C:
                    case SDLK_X:
                    case SDLK_Z:
                    case SDLK_SELECT:
                    case SDLK_SPACE:
                        run_cartridge(renderer);
                        return true;
                    case SDLK_LEFT:
                        select_prev_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                    case SDLK_RIGHT:
                        select_next_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                }
            }
            else if (state == STATE_EMULATOR)
            {
                switch (event->key.key)
                {
                    case SDLK_EQUALS:
                        SDL_Log("Screen data CRC: 0x%x", crc32(pico8_ram, 0x6000, 0x2000));
                        break;
                    case SDLK_SOFTLEFT:
                    case SDLK_ESCAPE:
                        destroy_vm();
                        reset_memory();
                        state = STATE_MENU;
                        has_draw = false;
                        has_update = false;
                        has_update60 = false;
                        return true;
                }
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            const SDL_JoystickID which = event->gbutton.which;

            if (state == STATE_MENU)
            {
                switch (event->gbutton.button)
                {
                    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                        select_prev_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                        select_next_cartridge(renderer);
                        render_cartridge(renderer);
                        return true;
                    case SDL_GAMEPAD_BUTTON_EAST:
                    case SDL_GAMEPAD_BUTTON_SOUTH:
                        run_cartridge(renderer);
                        return true;
                }
            }
            else if (state == STATE_EMULATOR)
            {
                switch (event->gbutton.button)
                {
                    case SDL_GAMEPAD_BUTTON_BACK:
                    case SDL_GAMEPAD_BUTTON_START:
                        destroy_vm();
                        reset_memory();
                        state = STATE_MENU;
                        has_draw = false;
                        has_update = false;
                        has_update60 = false;
                        return true;
                }
                break;
            }
        }
        case SDL_EVENT_FINGER_DOWN:
        {
            if (state == STATE_MENU)
            {
                // Use drawable size (physical pixels) for high-DPI displays
                int drawable_w, drawable_h;
                SDL_GetRenderOutputSize(renderer, &drawable_w, &drawable_h);

                // Get scale factor from current cart_rect.
                int scale = (int)(cart_rect.w / 160.0f);
                if (scale < 1) scale = 1;

                // Convert normalized touch coordinates to pixel coordinates
                // within the scaled display area.
                float touch_x = event->tfinger.x * drawable_w;
                float touch_y = event->tfinger.y * drawable_h;

                // Translate to coordinates relative to the cart rendering area.
                float local_x = touch_x - cart_rect.x;
                float local_y = touch_y - cart_rect.y;

                // Scale back to the virtual 160x205 coordinate system.
                float virt_x = local_x / scale;
                float virt_y = local_y / scale;

                // Check against touch regions.
                int touch_count = 0;
                const touch_region* regions = get_touch_regions(&touch_count);

                for (int i = 0; i < touch_count; i++)
                {
                    if (point_in_touch_region(virt_x, virt_y, &regions[i]))
                    {
                        uint8_t bit = regions[i].bit;

                        // Bit 0 = Left, Bit 1 = Right
                        if (bit == 0)
                        {
                            select_prev_cartridge(renderer);
                            render_cartridge(renderer);
                            return true;
                        }
                        else if (bit == 1)
                        {
                            select_next_cartridge(renderer);
                            render_cartridge(renderer);
                            return true;
                        }
                        // Bits 2-5 are Up/Down/Button O/Button X which map to "run".
                        else if (bit == 2 || bit == 3 || bit == 4 || bit == 5)
                        {
                            run_cartridge(renderer);
                            return true;
                        }
                        break;
                    }
                }
            }
            else if (state == STATE_EMULATOR && screen_rect.w > 0)
            {
                // Fallback event-based touch handling for emulator mode
                // Combined with polling-based approach for better reliability.
                int drawable_w, drawable_h;
                SDL_GetRenderOutputSize(renderer, &drawable_w, &drawable_h);

                if (drawable_w > 0 && drawable_h > 0)
                {
                    // Get scale factor from screen_rect.
                    int scale = (int)(screen_rect.w / 128.0f);
                    if (scale < 1) scale = 1;

                    // Convert normalized touch coordinates to physical pixel coordinates.
                    float touch_x = event->tfinger.x * drawable_w;
                    float touch_y = event->tfinger.y * drawable_h;

                    // Translate to coordinates relative to the game rendering area (screen_rect).
                    float local_x = touch_x - screen_rect.x;
                    float local_y = touch_y - screen_rect.y;

                    // Scale back to virtual 128x128 game coordinate system.
                    float virt_x = local_x / scale;
                    float virt_y = local_y / scale;

                    // Map to the touch regions (which are in the 160x205 space)
                    // but offset to the game area (which starts at 16, 24 in that space).
                    float region_x = virt_x + 16.0f;  // Game area X offset
                    float region_y = virt_y + 24.0f;  // Game area Y offset

                    // Check against touch regions and set button state.
                    int touch_count = 0;
                    const touch_region* regions = get_touch_regions(&touch_count);

                    for (int i = 0; i < touch_count; i++)
                    {
                        if (point_in_touch_region(region_x, region_y, &regions[i]))
                        {
                            touch_button_state |= (1 << regions[i].bit);

                            uint8_t bit = regions[i].bit;
                            if (bit == 99)
                            {
                                state = STATE_MENU;
                                continue;
                            }
                        }
                    }
                }
            }
            break;
        }
        case SDL_EVENT_FINGER_UP:
        {
            if (state == STATE_EMULATOR && screen_rect.w > 0)
            {
                // Fallback event-based touch handling for emulator mode
                // Clear button state for released finger.
                int drawable_w, drawable_h;
                SDL_GetRenderOutputSize(renderer, &drawable_w, &drawable_h);

                if (drawable_w > 0 && drawable_h > 0)
                {
                    // Get scale factor from screen_rect.
                    int scale = (int)(screen_rect.w / 128.0f);
                    if (scale < 1) scale = 1;

                    // Convert normalized touch coordinates to physical pixel coordinates.
                    float touch_x = event->tfinger.x * drawable_w;
                    float touch_y = event->tfinger.y * drawable_h;

                    // Translate to coordinates relative to the game rendering area (screen_rect).
                    float local_x = touch_x - screen_rect.x;
                    float local_y = touch_y - screen_rect.y;

                    // Scale back to virtual 128x128 game coordinate system.
                    float virt_x = local_x / scale;
                    float virt_y = local_y / scale;

                    // Map to the touch regions (which are in the 160x205 space)
                    // but offset to the game area (which starts at 16, 24 in that space).
                    float region_x = virt_x + 16.0f;  // Game area X offset
                    float region_y = virt_y + 24.0f;  // Game area Y offset

                    // Check against touch regions and clear button state.
                    int touch_count = 0;
                    const touch_region* regions = get_touch_regions(&touch_count);

                    for (int i = 0; i < touch_count; i++)
                    {
                        if (point_in_touch_region(region_x, region_y, &regions[i]))
                        {
                            touch_button_state &= ~(1 << regions[i].bit);
                        }
                    }
                }
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            // When SDL_HINT_MOUSE_TOUCH_EVENTS is enabled, touch input comes as mouse events.
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                SDL_Window* window = SDL_GetRenderWindow(renderer);
                int window_w, window_h;

                if (!SDL_GetWindowSize(window, &window_w, &window_h))
                {
                    SDL_Log("Unable to get window size: %s", SDL_GetError());
                    break;
                }

                // Get mouse position (which is touch position when using MOUSE_TOUCH_EVENTS).
                float mouse_x = event->button.x;
                float mouse_y = event->button.y;

                if (state == STATE_MENU)
                {
                    // Translate to coordinates relative to the cart rendering area.
                    float local_x = mouse_x - cart_rect.x;
                    float local_y = mouse_y - cart_rect.y;

                    // Calculate scale from cart_rect
                    int scale = (int)(cart_rect.w / 160.0f);
                    if (scale < 1) scale = 1;

                    // Scale to virtual 160x205 coordinate system.
                    float virt_x = local_x / scale;
                    float virt_y = local_y / scale;

                    // Map to touch regions
                    int touch_count = 0;
                    const touch_region* regions = get_touch_regions(&touch_count);

                    for (int i = 0; i < touch_count; i++)
                    {
                        if (point_in_touch_region(virt_x, virt_y, &regions[i]))
                        {
                            uint8_t bit = regions[i].bit;

                            // Bit 0 = Left, Bit 1 = Right
                            if (bit == 0)
                            {
                                select_prev_cartridge(renderer);
                                render_cartridge(renderer);
                                return true;
                            }
                            else if (bit == 1)
                            {
                                select_next_cartridge(renderer);
                                render_cartridge(renderer);
                                return true;
                            }
                            // Bits 2-5 are Up/Down/Button O/Button X which map to "run"
                            else if (bit == 2 || bit == 3 || bit == 4 || bit == 5)
                            {
                                run_cartridge(renderer);
                                return true;
                            }
                            break;
                        }
                    }
                }
                else if (state == STATE_EMULATOR && screen_rect.w > 0)
                {
                    // During emulation, detect touch on game buttons and update button state
                    // Translate to coordinates relative to the game rendering area (screen_rect).
                    float local_x = mouse_x - screen_rect.x;
                    float local_y = mouse_y - screen_rect.y;

                    // Calculate scale from screen_rect.
                    int scale = (int)(screen_rect.w / 128.0f);
                    if (scale < 1) scale = 1;

                    // Scale to virtual 128x128 game coordinate system.
                    float virt_x = local_x / scale;
                    float virt_y = local_y / scale;

                    // Map to the touch regions (which are in the 160x205 space)
                    // but offset to the game area (which starts at 16, 24 in that space).
                    float region_x = virt_x + 16.0f;  // Game area X offset
                    float region_y = virt_y + 24.0f;  // Game area Y offset

                    int touch_count = 0;
                    const touch_region* regions = get_touch_regions(&touch_count);

                    for (int i = 0; i < touch_count; i++)
                    {
                        if (point_in_touch_region(region_x, region_y, &regions[i]))
                        {
                            touch_button_state |= (1 << regions[i].bit);

                            uint8_t bit = regions[i].bit;
                            if (bit == 99)
                            {
                                state = STATE_MENU;
                                continue;
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            // Clear touch button state when touch is released.
            if (event->button.button == SDL_BUTTON_LEFT && state == STATE_EMULATOR)
            {
                touch_button_state = 0;
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
        Uint64 frame_start = SDL_GetTicks();
        Uint32 frame_ms = has_update60 ? 1000u / 60u : 1000u / 30u;
        // Expose timing to API for stat(1) CPU usage reporting.
        pico8_frame_start = frame_start;
        pico8_frame_ms = frame_ms;

        if (has_update)
        {
            update_input(renderer);
            update_touch_input(renderer);
            call_pico8_function(vm, "_update");
        }
        else if (has_update60)
        {
            update_input(renderer);
            update_touch_input(renderer);
            call_pico8_function(vm, "_update60");
        }

        if (has_draw)
        {
            reset_draw_state();
            call_pico8_function(vm, "_draw");
        }

        update_time();
        render_cartridge(renderer);
        update_from_virtual_memory(renderer);
        SDL_RenderPresent(renderer);

#ifndef __SYMBIAN32__
        Uint64 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < frame_ms)
        {
            SDL_Delay((Uint32)(frame_ms - elapsed));
        }
#endif

        return true;
    }
#ifndef __SYMBIAN32__
    SDL_Delay(1);
#endif

    return true;
}
