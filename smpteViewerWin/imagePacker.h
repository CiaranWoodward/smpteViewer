#pragma once
#include <stdint.h>
#include "packetGetter.h"
#include <string>

struct SMPTE2022_metadata {
	uint8_t map; // 0 = direct sample; 1 = SMPTE ST 425-1 Level B-DL; 2 = SMPTE ST 425-1 Level B-DS;
	int xRes, yRes;
	bool isInterlaced;
	uint8_t frate;
	uint8_t frame;
	uint8_t sample;
};

class imagePacker
{
public:
	imagePacker(std::string & filepath);
	void init(int *width, int *height);
	uint8_t* getNextPixels();
	~imagePacker();
private:
	uint8_t * pixelBuf;
	int width, height, pixelBufLen;
	packetGetter mPacketGetter;
	struct SMPTE2022_metadata mMetadata;
	bool isInterlaced;
};

