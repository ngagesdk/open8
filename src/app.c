/** @file app.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"

#include "app.h"

static SDL_AudioDeviceID audio_device;

bool init_app(SDL_Renderer** renderer, SDL_Window* window)
{
	SDL_SetHint("SDL_RENDER_VSYNC", "1");
	SDL_SetHint("SDL_RENDER_NGAGE_SHOW_FPS", "1");
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
	SDL_SetAppMetadata("Pico-8", "1.0", "com.open8.ngagesdk");

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return false;
	}

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
	{
		SDL_Log("Couldn't initialize gamepad subsystem: %s", SDL_GetError());
	}

	window = SDL_CreateWindow("open8", 160 * 2, 205 * 2, SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!window)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return false;
	}
	SDL_HideCursor();

	*renderer = SDL_CreateRenderer(window, 0);
	if (!*renderer)
	{
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		return false;
	}

	int native_w, native_h;
	SDL_GetRenderOutputSize(*renderer, &native_w, &native_h);

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

	cart_rect.x = (native_w - window_w) / 2;
	cart_rect.y = (native_h - window_h) / 2;
	cart_rect.w = window_w;
	cart_rect.h = window_h;

	screen_rect.x = cart_rect.x + (scale * 16);
	screen_rect.y = cart_rect.y + (scale * 24);
	screen_rect.w = (scale * 128);
	screen_rect.h = (scale * 128);

	SDL_AudioSpec spec;
	spec.channels = 1;
	spec.format = SDL_AUDIO_S16;
	spec.freq = 8000;

	audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	if (audio_device == 0)
	{
		SDL_Log("SDL_OpenAudioDevice: %s", SDL_GetError());
		return false;
	}

	return true;
}

void destroy_app(void)
{
	SDL_CloseAudioDevice(audio_device);
}
