#pragma once
#include <SDL.h>
#undef main

#include <string>

class pcapGenerator
{
public:
	pcapGenerator(int mode, int timeSec, std::string filepath);
	~pcapGenerator();
private:
	int xDim;
	int yDim;
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	uint8_t* pixels;
};

