#pragma once
#include <stdint.h>
#include "packetGetter.h"
#include <string>

class imagePacker
{
public:
	imagePacker(std::string & filepath);
	void init(int width, int height);
	uint8_t* getNextPixels();
	~imagePacker();
private:
	uint8_t * pixelBuf;
	int width, height, pixelBufLen;
	packetGetter mPacketGetter;
};

