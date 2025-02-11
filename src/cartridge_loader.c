/* @file cartridge_loader.c
 *
 * A Pico-8 emulator for the Nokia N-Gage.
 */

#include <SDL3/SDL.h>
#include "cartridge_loader.h"
#include "pico_defs.h"

#include <dirent.h>
#include <string.h>

#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static char** available_carts;
static int num_carts;

static cartridge_t cartridge;
static int selection;

static int load_cartridge(const char* filename, cartridge_t* cartridge);
static void destroy_cartridge(cartridge_t* cartridge);

bool init_cartridge_loader(void)
{
    num_carts = 0;
    selection = 0;

    char path[256];
    SDL_snprintf(path, sizeof(path), "%scarts", SDL_GetBasePath());

    DIR* dir = opendir(path);
    if (!dir)
    {
        SDL_Log("Couldn't open directory: %s", path);
        return;
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
    load_cartridge(available_carts[0], &cartridge);
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
}

void next_cartridge(void)
{
    destroy_cartridge(&cartridge);
    selection++;
    if (selection >= num_carts)
    {
        selection = 0;
    }
    load_cartridge(available_carts[selection], &cartridge);
}

void prev_cartridge(void)
{
    destroy_cartridge(&cartridge);
    selection--;
    if (selection < 0)
    {
        selection = num_carts - 1;
    }
    load_cartridge(available_carts[selection], &cartridge);
}

void update_cartridges()
{
    SDL_FRect source;
    SDL_FRect dest;

    source.x = 16.f;
    source.y = 24.f;
    source.w = SCREEN_SIZE;
    source.h = SCREEN_SIZE;

    dest.x = 24.f;
    dest.y = 20.f;
    dest.w = SCREEN_SIZE;
    dest.h = SCREEN_SIZE;

    extern SDL_Renderer* renderer;

    SDL_RenderTexture(renderer, cartridge.image, &source, &dest);
    SDL_RenderPresent(renderer);
}

static int load_cartridge(const char* filename, cartridge_t* cartridge)
{
   int width, height, bpp;
   char path[256];

   SDL_snprintf(path, sizeof(path), "%scarts/%s", SDL_GetBasePath(), filename);
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

   // tbd.

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
   stbi_image_free(image_data);

   extern SDL_Renderer* renderer;
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
