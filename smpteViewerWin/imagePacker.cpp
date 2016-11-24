#include "imagePacker.h"
#include <stdlib.h>

static uint8_t counter, counterColour = 0;

imagePacker::imagePacker()
{	
	pixelBuf = NULL;
}

void imagePacker::init(int width, int height)
{
	this->width = width;
	this->height = height;
	this->pixelBufLen = width * height * 2;
	pixelBuf = (uint8_t *)malloc(pixelBufLen);
}

uint8_t * imagePacker::getNextPixels()
{
	bool processingRemaining = true;
	while (processingRemaining) {
		uint8_t * packet = mPacketGetter.getNextPacket();
		if (packet[43] & 0x80) processingRemaining = false; //Marker is set, final packet of frame
		int curOffset = 62;
		uint8_t bitOffset = 0;

		while (curOffset < 1437) {
			//Bitwise voodoo to extract the 10bit values from the 16 bit stream
			uint16_t curDectet = 0;
			uint16_t mask1 = 0xFF >> bitOffset;
			uint16_t mask2 = 0xFF << (6 - bitOffset);
			curDectet += ((uint16_t) packet[curOffset++] & mask1 ) << (6 - bitOffset);
			curDectet += ((uint16_t) packet[curOffset] & mask2);

			bitOffset += 2;
			if (bitOffset == 10) {
				bitOffset = 0;
				curOffset++;
			}

			//Remove any special data
			//TODO: remove TLV data
			//TODO: Sync to frame starts and edges
			if (curDectet < 4 || curDectet > 1019) continue;

			//Reduce the range to 8 bits by removing the 2 LSB
			curDectet = curDectet >> 2;

			//TODO: Transfer the data into the pixel buffer in the correct order
		}
	}
	uint8_t * packet = NULL;
	for (int i = 0; i < pixelBufLen; i += 2) {
		pixelBuf[i] = counter++;
		pixelBuf[i + 1] = counterColour;
	}
	counterColour++;

	return pixelBuf;
}


imagePacker::~imagePacker()
{
	if (pixelBuf) {
		free(pixelBuf);
	}
}
