/* @file cartridge_loader.c
 *
 * A Pico-8 emulator for the Nokia N-Gage.
 */

#include <SDL3/SDL.h>

typedef struct cartridge
{
    SDL_Texture* image;
    Uint8* data;
    Uint32 size;

    Uint8 cart_data[0x8020];

} cartridge_t;

bool init_cartridge_loader(SDL_Renderer* renderer);
void destroy_cartridge_loader(void);
void next_cartridge(SDL_Renderer* renderer);
void prev_cartridge(SDL_Renderer* renderer);
void render_cartridge(SDL_Renderer* renderer, bool with_frame);
