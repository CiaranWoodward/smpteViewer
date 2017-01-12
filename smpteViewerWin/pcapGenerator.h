#pragma once
#include <SDL.h>
#undef main

#include "imageGenerator.h"
#include <string>
#include <fstream>

#define PACKETSIZE 1438

const uint8_t pktHeaderLength = 42;
const uint8_t rtpHeaderLength = 12;
const uint8_t hbrmHeaderLength = 8;

#define DEFCRC 0b00 0000 0000 0011 0001 //0x00031
#define DEFRCREV 0b10 0011 0000 0000 0000 //0x23000

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

	unsigned long int maxFrames;

	unsigned long int curFrameCount;
	uint16_t sequenceNumber;
	unsigned long long int curDectetCount;
	unsigned long long int clockSpeed;
	unsigned long long int startTime;

	uint8_t * chromaBuf;
	uint8_t * lumaBuf;
	unsigned int chromaCount;
	unsigned int lumaCount;

	unsigned int pktCursor;
	unsigned int lineNo;
	uint8_t bitOffset;
	uint8_t pkt[PACKETSIZE];

	bool isInterlaced;
	bool isHD;

	std::ofstream outstr;

	void resetPacket();
	void pushPacket();
	void resetCRCBufs();
	void pushChromaDectet(uint16_t dectet);
	void pushLumaDectet(uint16_t dectet);
	void pushDectet(uint16_t dectet);
	void pushVerticalBlankingLine();
	void pushHorizBlankData();
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

