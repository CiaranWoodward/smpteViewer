#include "pcapGenerator.h"
#include "debugUtil.h"
#include <fstream>

#define PACKETSIZE 1437

pcapGenerator::pcapGenerator(int mode, int timeSec, std::string filepath)
{
	this->filepath = filepath;
	if (mode == 1) {
		xDim = 720;
		yDim = 486;
		mImageGenerator = new imageGenerator(xDim, yDim);
	}
}

void pcapGenerator::start()
{
	screen = SDL_CreateWindow(
		"Generating...",
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

	//TODO open file and write pcap header
	std::ofstream outstr = std::ofstream(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

	uint8_t header[24] = { 
						0xd4, 0xc3, 0xb2, 0xa1, //magic number
						0x00, 0x02,	//major version
						0x00, 0x04, //minor version
						0x00, 0x00, 0x00, 0x00, //GMT to local time correction
						0x00, 0x00, 0x00, 0x00, //sigfigs
						0x00, 0x00, 0xff, 0xff, //snaplen
						0x00, 0x00, 0x00, 0x01  //ethernet
	};
	outstr.write((char *)header, 24);

	const uint8_t pktHeaderLength = 42;
	uint8_t pktHeader[pktHeaderLength] = { 0x01, 0x00, 0x5e, 0x00, 0x27, 0x7a, 0x40, 0xa3, 0x6b, 0xa0, 0x01, 0xfe, 0x08, 0x00, 0x45, 0x02, 0x05, 0x90, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x36, 0xbe, 0xc0, 0xa8, 0x27, 0x7a, 0xef, 0x00, 0x27, 0x7a, 0x27, 0x10, 0x4e, 0x20, 0x05, 0x7c, 0x00, 0x00 };
	
	const uint8_t rtpHeaderLength = 12;
	const uint8_t hbrmHeaderLength = 8;

	uint8_t pkt[PACKETSIZE];
	memcpy(pkt, pktHeader, pktHeaderLength);

	unsigned long long int curDectet = 0;

	while (1) {
		pixels = mImageGenerator->getNextFrame();

		SDL_UpdateTexture(texture, NULL, pixels, xDim * 2);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		//process image into packets and write to file


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


pcapGenerator::~pcapGenerator()
{
}
