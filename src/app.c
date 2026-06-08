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
static int scale;
static int window_w;
static int window_h;
static int window_offset_x;
static int window_offset_y;

int get_scale(void)
{
#ifdef __SYMBIAN32__
	return 1;
#else
	return scale;
#endif
}

void get_window_offset(int* x, int* y)
{
#ifdef __SYMBIAN32__
	if (x) {
		*x = 8;
	}
	if (y) {
		*y = 1;
	}
#else
	if (x) {
		*x = window_offset_x;
	}
	if (y) {
		*y = window_offset_y;
	}
#endif
}

void get_window_size(int* w, int* h)
{
#ifdef __SYMBIAN32__
	if (w) {
		*w = 176;
	}
	if (h) {
		*h = 208;
	}
#else
	if (w) {
		*w = window_w;
	}
	if (h) {
		*h = window_h;
	}
#endif
}

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

	SDL_DisplayID display = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
	if (!mode)
	{
		SDL_Log("Could not get desktop display mode: %s", SDL_GetError());
		return false;
	}

	int native_w = mode->w;
	int native_h = mode->h;

	int base_w = 128;
	int base_h = 128;

	int scale_x = native_w / base_w;
	int scale_y = native_h / base_h;

	scale = (scale_x < scale_y) ? scale_x : scale_y;
	scale = scale - 1; // Leave some room for the window border and title bar.
	if (scale < 1)
	{
		scale = 1;
	}

	window_w = base_w * scale;
	window_h = base_h * scale;

	window_offset_x = (mode->w - window_w) / 2;
	window_offset_y = (mode->h - window_h) / 2;

	window = SDL_CreateWindow("open8", mode->w, mode->h, SDL_WINDOW_FULLSCREEN);
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
