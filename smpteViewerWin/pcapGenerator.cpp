#include "pcapGenerator.h"
#include "debugUtil.h"
#include <fstream>
#include <chrono>

#define CLOCKSPEED 27000000.0
#define CLOCKPERIOD ( 1.0 / (CLOCKSPEED))

#define BLANKLUMA 0x040
#define BLANKCHROMA 0x200


pcapGenerator::pcapGenerator(int mode, int timeSec, std::string filepath)
{
	this->filepath = filepath;
	if (mode == 1) {
		xDim = 720;
		yDim = 486;
		numPixels = xDim * yDim * 2;
		mImageGenerator = new imageGenerator(xDim, yDim);
		dectetsPerFrame = 900900; //(CLOCKSPEED) / (30/1.001)
		dectetsPerLine = 1716;
		hBlankLen = 276 - (4 * 2); //subtract the length of the EAV and SAV markers 
		isInterlaced = true;
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
						0x02, 0x00,	//major version
						0x04, 0x00, //minor version
						0x00, 0x00, 0x00, 0x00, //GMT to local time correction
						0x00, 0x00, 0x00, 0x00, //sigfigs
						0xFF, 0xFF, 0x00, 0x00, //snaplen
						0x01, 0x00, 0x00, 0x00  //ethernet
	};
	outstr.write((char *)header, 24);

	uint8_t pktHeader[pktHeaderLength] = { 0x01, 0x00, 0x5e, 0x00, 0x27, 0x7a, 0x40, 0xa3, 0x6b, 0xa0, 0x01, 0xfe, 0x08, 0x00, 0x45, 0x02, 0x05, 0x90, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x36, 0xbe, 0xc0, 0xa8, 0x27, 0x7a, 0xef, 0x00, 0x27, 0x7a, 0x27, 0x10, 0x4e, 0x20, 0x05, 0x7c, 0x00, 0x00 };
	
	memset(pkt, 0, PACKETSIZE);
	memcpy(pkt, pktHeader, pktHeaderLength);
	pktCursor = pktHeaderLength + rtpHeaderLength + hbrmHeaderLength;

	unsigned long long int curDectetCount = 0;

	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::high_resolution_clock::now();

	int numVblankLines = (dectetsPerFrame / dectetsPerLine) - yDim;

	while (1) {
		uint32_t curPixel = 0;
		uint32_t curPacketByte = pktHeaderLength;
		pixels = mImageGenerator->getNextFrame();

		SDL_UpdateTexture(texture, NULL, pixels, xDim * 2);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		uint32_t hblanklen;

		//process image into packets and write to file
		bool isBlanking = true;
		bool blankLumaToggle = false;
		uint16_t curDectet;
		while (curPixel < numPixels) {

			if (isBlanking) {
				curDectet = blankLumaToggle ? BLANKLUMA: BLANKCHROMA;
				blankLumaToggle = !blankLumaToggle;
			}
			else {
				//TODO: insert actual video data
			}

		}

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

void pcapGenerator::pushPacket()
{
	//TODO: write Pcap packet header
	//TODO: write to file

	//reset cursor zero the data section of packet ready for next filling
	pktCursor = pktHeaderLength + rtpHeaderLength + hbrmHeaderLength;
	memset(pkt + pktCursor, 0, PACKETSIZE - pktCursor);
}

void pcapGenerator::pushDectet(uint16_t dectet)
{
	uint8_t toInsert[2] = { 0 }; //TODO: Fill this

	for (int i = 0; i < 2; i++) {
		if (pktCursor < PACKETSIZE) {
			//TODO: Put toInsert[i] in this packet (using bitwise or)
		}
		else {
			//TODO: send current packet to file
		}
	}
}

void pcapGenerator::pushSAV(bool f, bool v)
{
	pushDectet(0x3FF);
	pushDectet(0);
	pushDectet(0);
	uint16_t xyz = 0;
	if (f) {
		if (v) {
			      //1fvhpppp00
			xyz = 0b1110110000;
		}
		else {
			xyz = 0b1100011100;
		}
	}
	else {
		if (v) {
			xyz = 0b1010101100;
		}
		else {
			xyz = 0b1000000000;
		}
	}
	pushDectet(xyz);
}

void pcapGenerator::pushEAV(bool f, bool v)
{
	pushDectet(0x3FF);
	pushDectet(0);
	pushDectet(0);
	uint16_t xyz = 0;
	if (f) {
		if (v) {
			xyz = 0b1111000100;
		}
		else {
			xyz = 0b1101101000;
		}
	}
	else {
		if (v) {
			xyz = 0b1011011000;
		}
		else {
			xyz = 0b1001110100;
		}
	}
	pushDectet(xyz);
}

pcapGenerator::~pcapGenerator()
{
}
