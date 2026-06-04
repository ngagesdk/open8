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
#include "memory.h"

#define STBI_ONLY_PNG
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

#ifdef _WIN32
#include "misc/dirent.h"
#else
#include <dirent.h>
#endif
#include <string.h>

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
	size_t count = 0;

	// Count occurrences of preset fill-patterns.
	for (size_t i = 0; i < cart->code_size; i++)
	{
		if ((uint8_t)cart->code[i] >= 128 && (uint8_t)cart->code[i] <= 135)
		{
			count++;
		}
	}

	if (count == 0)
	{
		return; // No changes needed.
	}

	// Compute new size with added quotes.
	size_t new_size = cart->code_size + count * 2;
	uint8_t* new_code = (uint8_t*)SDL_realloc(cart->code, new_size + 1); // +1 for null terminator.
	if (!new_code)
	{
		SDL_Log("Memory reallocation failed!");
		return;
	}

	cart->code = new_code;

	// Insert quotes while shifting data from end to start.
	char* src = cart->code + cart->code_size - 1;
	char* dest = cart->code + new_size;
	*dest-- = '\0'; // Null-terminate the new string.

	for (size_t i = cart->code_size; i-- > 0;)
	{
		if ((unsigned char)*src >= 128 && (unsigned char)*src <= 135)
		{
			*dest-- = '"';
			*dest-- = *src;
			*dest-- = '"';
		}
		else
		{
			*dest-- = *src;
		}
		src--;
	}

	cart->code_size = new_size;
}

// Replace PICO-8 button glyph tokens with their numeric button index.
// Cartridges compressed in .p8.png use single-byte P8SCII values for glyphs;
// plain .p8 text files use multi-byte UTF-8 sequences for the same glyphs.
// Both forms are handled here so that e.g. btn(x) becomes btn(5).
//
// PICO-8 button indices: left=0  right=1  up=2  down=3  o=4  x=5
//
// P8SCII single-byte values (standard PICO-8 encoding):
//   0x83 Down  0x8b Left  0x8e O  0x91 Right  0x94 Up  0x97 X
static void patch_button_glyphs(cart_t* cart)
{
	// Multi-byte UTF-8 sequences (longest first to avoid partial matches).
	static const struct { const uint8_t* seq; size_t len; char repl; } utf8_glyphs[] = {
		{ (const uint8_t*)"\xf0\x9f\x85\xbe\xef\xb8\x8f", 7, '4' }, // O (U+1F17E + VS16)
		{ (const uint8_t*)"\xe2\x9d\x8e\xef\xb8\x8f",     6, '5' }, // X (U+274E  + VS16)
		{ (const uint8_t*)"\xf0\x9f\x85\xbe",             4, '4' }, // O
		{ (const uint8_t*)"\xe2\xac\x85",                 3, '0' }, // Left.
		{ (const uint8_t*)"\xe2\x9e\xa1",                 3, '1' }, // Right.
		{ (const uint8_t*)"\xe2\xac\x86",                 3, '2' }, // Up.
		{ (const uint8_t*)"\xe2\xac\x87",                 3, '3' }, // Down.
		{ (const uint8_t*)"\xe2\x9d\x8e",                 3, '5' }, // X
	};
	static const size_t utf8_count = sizeof(utf8_glyphs) / sizeof(utf8_glyphs[0]);

	uint8_t* src = cart->code;
	size_t src_len = cart->code_size;

	// Allocate worst-case output buffer (same size as input; replacements only shrink).
	uint8_t* out = (uint8_t*)SDL_malloc(src_len + 1);
	if (!out)
	{
		SDL_Log("patch_button_glyphs: out of memory");
		return;
	}

	size_t si = 0, oi = 0;
	while (si < src_len)
	{
		uint8_t b = src[si];

		// Single-byte P8SCII button glyphs (from .p8.png decompressed code).
		if (b == 0x83) { out[oi++] = '3'; si++; continue; } // Down.
		if (b == 0x8b) { out[oi++] = '0'; si++; continue; } // Left.
		if (b == 0x8e) { out[oi++] = '4'; si++; continue; } // O
		if (b == 0x91) { out[oi++] = '1'; si++; continue; } // Right.
		if (b == 0x94) { out[oi++] = '2'; si++; continue; } // Up.
		if (b == 0x97) { out[oi++] = '5'; si++; continue; } // X

		// Multi-byte UTF-8 button glyphs (from plain .p8 text files).
		bool matched = false;
		if (b >= 0xe2)
		{
			for (size_t g = 0; g < utf8_count; g++)
			{
				size_t glen = utf8_glyphs[g].len;
				if (si + glen <= src_len && SDL_memcmp(src + si, utf8_glyphs[g].seq, glen) == 0)
				{
					out[oi++] = (uint8_t)utf8_glyphs[g].repl;
					si += glen;
					matched = true;
					break;
				}
			}
		}
		if (!matched)
		{
			out[oi++] = src[si++];
		}
	}
	out[oi] = '\0';

	// Replace cart code buffer with the patched version.
	SDL_free(cart->code);
	cart->code = out;
	cart->code_size = oi;
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
		patch_button_glyphs(cart);
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
	SDL_SetRenderTarget(renderer, NULL);

	SDL_FRect dest;

	dest.x = FRAME_OFFSET_X;
	dest.y = FRAME_OFFSET_Y;
	dest.w = FRAME_W;
	dest.h = FRAME_H;

	SDL_SetRenderDrawColor(renderer, 0x31, 0x31, 0x31, 0xff);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, cart.image, NULL, &dest);
}

bool init_core(SDL_Renderer* renderer)
{
	char path[256];

	num_carts = 0;
	selection = 0;
	prev_selection = !selection;

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
}

bool handle_events(SDL_Renderer* renderer, SDL_Event* event)
{
	switch (event->type)
	{
	case SDL_EVENT_QUIT:
	{
		return false;
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
			case SDLK_SOFTLEFT:
			case SDLK_ESCAPE:
				return false;
			case SDLK_5:
			case SDLK_KP_5:
			case SDLK_SELECT:
			case SDLK_SPACE:
				run_cartridge(renderer);
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
				return true;
			}
		}
		break;
	}
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	{
		const SDL_JoystickID which = event->gbutton.which;
		SDL_Log("Gamepad #%" SDL_PRIu32 " button %s -> %s", which, SDL_GetGamepadStringForButton(event->gbutton.button), event->gbutton.down ? "PRESSED" : "RELEASED");

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
			case SDL_GAMEPAD_BUTTON_EAST: // TODO: A / Circle
				run_cartridge(renderer);
				return true;
			}
		}
		else if (state == STATE_EMULATOR)
		{
			switch (event->gbutton.button)
			{
			case SDL_GAMEPAD_BUTTON_SOUTH: // TODO: B / Cross
				destroy_vm();
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
#ifndef __SYMBIAN32__
		Uint64 frame_start = SDL_GetTicks();
		Uint32 frame_ms = has_update60 ? 1000u / 60u : 1000u / 30u;
#endif

		if (has_update)
		{
			update_input();
			call_pico8_function(vm, "_update");
		}
		else if (has_update60)
		{
			update_input();
			call_pico8_function(vm, "_update60");
		}

		if (has_draw)
		{
			reset_draw_state();
			call_pico8_function(vm, "_draw");
		}

		update_time();
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
