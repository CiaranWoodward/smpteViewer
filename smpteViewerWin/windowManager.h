#pragma once
#include <SDL.h>
#undef main

#include "imagePacker.h"

class windowManager
{
public:
	windowManager();
	void start();
	~windowManager();

private:
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	uint8_t* pixels;
	imagePacker mImagePacker;
};

