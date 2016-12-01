#include "windowManager.h"
#include "debugUtil.h"
#include <stdio.h>
#include <stdlib.h>

#define xDim 720
#define yDim 486

#define pixelBufferSize xDim*yDim*2

windowManager::windowManager()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		logerror("Could not initialize SDL");
		exit(1);
	}
}

void windowManager::start()
{
	pixels = (uint8_t *) malloc(pixelBufferSize);
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

	mImagePacker.init(xDim, yDim);

	while (1) {

		pixels = mImagePacker.getNextPixels();

		SDL_UpdateTexture(texture, NULL, pixels, xDim * 2);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

//		SDL_Delay(500);

		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			SDL_DestroyTexture(texture);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(screen);
			SDL_Quit();
			exit(0);
			break;
		default:
			break;
		}
	}
}

windowManager::~windowManager()
{
	SDL_Quit();
}
