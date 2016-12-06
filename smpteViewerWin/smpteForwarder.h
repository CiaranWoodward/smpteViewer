#pragma once
#include <SDL.h>
#undef main

#include <string>


class smpteForwarder
{
public:
	smpteForwarder(std::string filepath);
	void start();
	~smpteForwarder();
private:
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
};

