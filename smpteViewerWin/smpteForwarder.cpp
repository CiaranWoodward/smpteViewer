#include "smpteForwarder.h"
#include "debugUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

smpteForwarder::smpteForwarder(std::string filepath)
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		logerror("Could not initialize SDL");
		exit(1);
	}
}

void smpteForwarder::start()
{
	screen = SDL_CreateWindow(
		"Forwarding...",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
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

	while (1) {

		SDL_RenderPresent(renderer);

		SDL_WaitEvent(&event);
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

smpteForwarder::~smpteForwarder()
{
	SDL_Quit();
}