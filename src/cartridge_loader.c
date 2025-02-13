/* @file cartridge_loader.c
 *
 * A Pico-8 emulator for the Nokia N-Gage.
 */

#include <SDL3/SDL.h>
#include <stdio.h>
#include "lexaloffle/p8_compress.h"
#include "cartridge_loader.h"
#include "image_loader.h"
#include "pico_defs.h"
#include "stb_image.h"

#include <dirent.h>
#include <string.h>

static SDL_Texture* frame;

static char** available_carts;
static int num_carts;

static cartridge_t cartridge;
static int selection;

static int load_cartridge(SDL_Renderer* renderer, const char* file_name, cartridge_t* cartridge);
static void destroy_cartridge(cartridge_t* cartridge);
static void extract_pico8_data(const Uint8* image_data, Uint8* cart_data);

bool init_cartridge_loader(SDL_Renderer* renderer)
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
        available_carts = SDL_realloc(available_carts, (num_carts + 1) * sizeof(char*));
        available_carts[num_carts] = SDL_strdup(entry->d_name);
        num_carts++;
    }
    closedir(dir);

    if (!num_carts)
    {
        SDL_Log("No cartridges found in directory: %s", path);
        return false;
    }
    if (!load_cartridge(renderer, (const Uint8*)available_carts[0], &cartridge))
    {
        return false;
    }
    return true;
}

void destroy_cartridge_loader(void)
{
    destroy_cartridge(&cartridge);
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

void next_cartridge(SDL_Renderer* renderer)
{
    destroy_cartridge(&cartridge);
    selection++;
    if (selection >= num_carts)
    {
        selection = 0;
    }
    load_cartridge(renderer, available_carts[selection], &cartridge);
}

void prev_cartridge(SDL_Renderer* renderer)
{
    destroy_cartridge(&cartridge);
    selection--;
    if (selection < 0)
    {
        selection = num_carts - 1;
    }
    load_cartridge(renderer, available_carts[selection], &cartridge);
}

void render_cartridge(SDL_Renderer* renderer, bool with_frame)
{
    SDL_FRect source;
    SDL_FRect dest;

    source.x = 16.f;
    source.y = 24.f;
    source.w = SCREEN_SIZE;
    source.h = SCREEN_SIZE;

    dest.x = 24.f;
    dest.y = 25.f;
    dest.w = SCREEN_SIZE;
    dest.h = SCREEN_SIZE;

    if (with_frame)
    {
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, frame, NULL, NULL);
    }
    SDL_RenderTexture(renderer, cartridge.image, &source, &dest);
    SDL_RenderPresent(renderer);
}

static int load_cartridge(SDL_Renderer* renderer, const char* file_name, cartridge_t* cartridge)
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

    cartridge->data = (Uint8*)SDL_calloc(file_size, sizeof(Uint8));
    if (!cartridge->data)
    {
        SDL_Log("Couldn't allocate memory for cartridge data");
        fclose(file);
        return 0;
    }
    fread(cartridge->data, 1, file_size, file);
    cartridge->size = file_size;
    fclose(file);

    Uint32* image_data = stbi_load_from_memory(cartridge->data, cartridge->size, &width, &height, &bpp, 4);
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

    extract_pico8_data(image_data, cartridge->cart_data);
    Uint32 header = *(Uint32*)&cartridge->cart_data[0x4300];
    if (0x003a633a == header) // :c: followed by \x00
    {
        // Code is compressed (old format).
        cartridge->code_size = decompress_mini(&cartridge->cart_data[0x4300], cartridge->code, MAX_CODE_SIZE);
        if (cartridge->code_size == 1 || !cartridge->code_size)
        {
            SDL_Log("Corrupt code data.");
        }
        else
        {
            SDL_snprintf(path, sizeof(path), "%scart.p8", SDL_GetUserFolder(SDL_FOLDER_SAVEDGAMES));
            FILE* code_file = fopen(path, "wb+");
            if (code_file)
            {
                fwrite(cartridge->code, 1, cartridge->code_size, code_file);
                fclose(code_file);
            }
        }
    }
    else if (0x61787000 == header) // \x00 followed by pxa
    {
        SDL_Log("Code is compressed (new format, v0.2.0+).");
    }
    else
    {
        SDL_Log("Code is in plaintext.");
    }

    stbi_image_free(image_data);

    cartridge->image = SDL_CreateTextureFromSurface(renderer, surface);
    if (!cartridge->image)
    {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return 0;
    }
    SDL_DestroySurface(surface);

    return 1;
}

static void destroy_cartridge(cartridge_t* cartridge)
{
    if (!cartridge)
    {
        return;
    }

    if (cartridge->image)
    {
        SDL_DestroyTexture(cartridge->image);
    }

    if (cartridge->data)
    {
        SDL_free(cartridge->data);
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
