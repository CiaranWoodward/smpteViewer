#pragma once
#include <SDL.h>
#undef main

#include <string>
#include "packetGetter.h"


class smpteForwarder
{
public:
	smpteForwarder(std::string ipStr, std::string portStr, std::string filepath);
	void start();
	~smpteForwarder();
private:
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	packetGetter mPacketGetter;
	uint32_t port;
	std::string ipAddr, portStr;
};

