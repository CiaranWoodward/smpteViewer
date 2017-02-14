#include "windowManager.h"
#include "debugUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

#define pixelBufferSize xDim*yDim*2

windowManager::windowManager(std::string filepath):
	mImagePacker(filepath)
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		logerror("Could not initialize SDL");
		exit(1);
	}
}

void windowManager::start()
{
	mImagePacker.init(&xDim, &yDim);

	if (xDim == 0 || yDim == 0) return;

	screen = SDL_CreateWindow(
		"SMPTE-2022 viewer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		xDim,
		yDim,
		0
	);
	if (!screen) {
		logerror("SDL: Could not create window");
		exit(1);
	}

	renderer = SDL_CreateRenderer(screen, -1, 0);
	if (!renderer) {
		logerror("SDL: Could not create renderer");
		exit(1);
	}

	//Add a YUV texture to draw to
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_YUY2,
		SDL_TEXTUREACCESS_STREAMING,
		xDim,
		yDim
	);
	if (!texture) {
		logerror("SDL: Could not create texture");
		exit(1);
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> prev = std::chrono::high_resolution_clock::now();
	int missedTimeCount = 0;

	while (1) {

		pixels = mImagePacker.getNextPixels();

		SDL_UpdateTexture(texture, NULL, pixels, xDim * 2);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		prev = prev + std::chrono::nanoseconds(33366666);

		std::this_thread::sleep_until(prev);
		/*
		if (std::chrono::duration_cast<std::chrono::nanoseconds>(prev-now).count() > 0) {
			std::this_thread::sleep_for(prev-now);
		}
		else {
			missedTimeCount++;
		}*/
		prev = std::chrono::high_resolution_clock::now();;

		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			SDL_DestroyTexture(texture);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(screen);
			SDL_Quit();
			return;
		default:
			break;
		}
	}
}

windowManager::~windowManager()
{
	SDL_Quit();
}
