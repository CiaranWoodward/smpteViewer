#pragma once
#include <SDL.h>
#undef main

#include "imagePacker.h"
#include <string>

class windowManager
{
public:
	windowManager(std::string filepath);
	void start();
	~windowManager();

private:
	int xDim;
	int yDim;
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	uint8_t* pixels;
	imagePacker mImagePacker;
};

