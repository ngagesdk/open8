/* @file cartridge_loader.c
 *
 * A Pico-8 emulator for the Nokia N-Gage.
 */

#include "SDL3/SDL.h"

typedef struct cartridge
{
    SDL_Texture* image;
    Uint8* data;
    Uint32 size;

} cartridge_t;

bool init_cartridge_loader(void);
void destroy_cartridge_loader(void);
void next_cartridge(void);
void prev_cartridge(void);
void update_cartridges();
