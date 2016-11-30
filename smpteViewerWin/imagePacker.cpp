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
	int curSDIDataCount = 0;
	int ignoreDectets = 0;
	int syncStart = 0, ancStart = 0;
	bool horizontalBlanking = 0;
	bool verticalBlanking = 0;
	bool skipFirstOctet = false;
	uint16_t curDectet = 0;
	uint8_t bitOffset = 0;
	while (processingRemaining) {
		const uint8_t * packet = mPacketGetter.getNextPacket();
		if (packet[43] & 0x80) processingRemaining = false; //Marker is set, final packet of frame
		int curOffset = 62;

		while (curOffset <= 1437) {
			//Bitwise voodoo to extract the 10bit values from the 16 bit stream
			uint8_t mask1 = 0xFF >> bitOffset;
			uint8_t mask2 = 0xFF << (6 - bitOffset);
			if(!skipFirstOctet) curDectet = ((uint16_t)(packet[curOffset++] & mask1)) << (2 + bitOffset);
			if (curOffset > 1437) {
				skipFirstOctet = true;
				continue;
			}
			curDectet += ((uint16_t)(packet[curOffset] & mask2)) >> (6-bitOffset);
			skipFirstOctet = false;

			bitOffset += 2;
			if (bitOffset == 8) {
				bitOffset = 0;
				curOffset++;
			}

			if (ignoreDectets) {
				ignoreDectets--;
				continue;
			}

			//Remove any special data
			//TODO: remove TLV data
			//TODO: Sync to frame starts and edges
			if (curDectet < 4 || curDectet > 1019 || ancStart || syncStart) {

				if (ancStart == 0) {
					if (curDectet == 0x3FF && syncStart == 0) syncStart = 1;
					else if (curDectet == 0x00 && syncStart == 1) syncStart = 2;
					else if (curDectet == 0x00 && syncStart == 2) syncStart = 3;
					else if (syncStart == 3) {
						syncStart = 0;
						if (curDectet & (1 << 7)) {	//V bit = Vertical blanking
							verticalBlanking = 1;
						}
						else {
							verticalBlanking = 0;
						}

						if (curDectet & (1 << 6)) {	//H bit = Horizontal blanking
							horizontalBlanking = 1;
						}
						else {
							horizontalBlanking = 0;
						}
					}
				}

				if (syncStart == 0) {
					if (curDectet == 0x00 && ancStart == 0) ancStart = 1;
					else if (curDectet == 0x3FF && ancStart == 1) ancStart = 2;
					else if (curDectet == 0x3FF && ancStart == 2) ancStart = 3;
					else if (ancStart == 3 || ancStart == 4) ancStart++;
					else if (ancStart == 5) {
						ignoreDectets = (curDectet & 0xFF) + 1;
						ancStart = 0;
					}
				}

				continue;
			}

			if (horizontalBlanking) continue;

			//Reduce the range to 8 bits by removing the 2 LSB
			curDectet = curDectet >> 2;

			//Transfer the data into the pixel buffer in the correct order
			//Cb -> U; Y -> Y; Cr -> V
			int pixelIndex = curSDIDataCount;
			pixelIndex += (curSDIDataCount % 2) ? -1 : 1;

			if (pixelIndex > pixelBufLen) {
				return pixelBuf; //this shouldn't happen
			}

			pixelBuf[pixelIndex] = curDectet;
			curSDIDataCount++;
		}
	}
	/*uint8_t * packet = NULL;
	for (int i = 0; i < pixelBufLen; i += 2) {
		pixelBuf[i] = counter++;
		pixelBuf[i + 1] = counterColour;
	}
	counterColour++;*/

	return pixelBuf;
}


imagePacker::~imagePacker()
{
	if (pixelBuf) {
		free(pixelBuf);
	}
}
