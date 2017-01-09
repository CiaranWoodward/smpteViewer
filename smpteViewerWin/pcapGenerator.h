#pragma once
#include <SDL.h>
#undef main

#include "imageGenerator.h"
#include <string>

#define PACKETSIZE 1437

const uint8_t pktHeaderLength = 42;
const uint8_t rtpHeaderLength = 12;
const uint8_t hbrmHeaderLength = 8;

class pcapGenerator
{
public:
	pcapGenerator(int mode, int timeSec, std::string filepath);
	void start();
	~pcapGenerator();
private:
	int xDim;
	int yDim;
	int numPixels;
	int hBlankLen;
	long int dectetsPerFrame;
	long int dectetsPerLine;

	unsigned int pktCursor;
	uint8_t bitOffset;
	uint8_t pkt[PACKETSIZE];

	bool isInterlaced;

	void pushPacket();
	void pushDectet(uint16_t dectet);
	void pushSAV(bool f, bool v);
	void pushEAV(bool f, bool v);

	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	uint8_t* pixels;
	std::string filepath;

	imageGenerator * mImageGenerator;
};

