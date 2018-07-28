#include "imagePacker.h"
#include "debugUtil.h"
#include <stdlib.h>

static uint8_t counter, counterColour = 0;

imagePacker::imagePacker(std::string & filepath):
	mPacketGetter(filepath)
{	
	pixelBuf = NULL;
}

void imagePacker::init(int *width, int *height)
{

	pkt_ll * pkt = mPacketGetter.getNextPacket();
	
	if ((pkt->pkt[43] & 0x7F) == 98) {	//Make sure is valid SMPTE2022-6 packet (yes, this is a very loose check)
		mMetadata.frate = (pkt->pkt[59] & 0x0F) << 4;
		mMetadata.frate += (pkt->pkt[60] & 0xF0) >> 4;

		mMetadata.sample = (pkt->pkt[60] & 0x0F);

		mMetadata.frame = (pkt->pkt[58] & 0x0F) << 4;
		mMetadata.frame += (pkt->pkt[59] & 0xF0) >> 4;

		mMetadata.map = (pkt->pkt[58] & 0xF0) >> 4;
	}
	else {
		logerror("Not pure SMPTE2022 file - aborted");
		*width = 0;
		*height = 0;
		return;
	}

	if (mMetadata.frame == 0x10) {
		this->width = 720;
		this->height = 486;
		isInterlaced = true;
		isHD = false;
	}
	else if (mMetadata.frame == 0x20) {
		this->width = 1920;
		this->height = 1080;
		isInterlaced = true;
		isHD = true;
	}
	else if (mMetadata.frame == 0x21) {
		this->width = 1920;
		this->height = 1080;
		isInterlaced = false;
		isHD = true;
	}
	else {
		logerror("Currently unsupported sdi format - aborted");
		*width = 0;
		*height = 0;
		return;
	}

	mPacketGetter.returnPacket(pkt);
	*width = this->width;
	*height = this->height;
	this->pixelBufLen = this->width * this->height * 2;
	pixelBuf = (uint8_t *)malloc(pixelBufLen);
}

uint8_t * imagePacker::getNextPixels()
{
	bool exit = false;
	bool processingRemaining = true;
	int curSDIDataCount = 0;
	int ignoreDectets = 0;
	int syncStart = 0, ancStart = 0;
	bool hdSplitter = false;
	bool horizontalBlanking = 0;
	bool verticalBlanking = 0;
	bool interleaved = 0;
	bool skipFirstOctet = false;
	uint16_t curDectet = 0;
	uint8_t bitOffset = 0;
	while (processingRemaining) {
		pkt_ll * pkt = mPacketGetter.getNextPacket();
		if (pkt == NULL) break; //Drop frame
		uint8_t * packet = pkt->pkt;
		uint8_t * payload = pkt->GetStartOfPayload();
		if (packet[43] & 0x80) processingRemaining = false; //Marker is set, final packet of frame
		int curOffset = 0;
		int payloadLen = 1376;

		while (curOffset < payloadLen) {
			//Bitwise voodoo to extract the 10bit values from the 16 bit stream
			uint8_t mask1 = 0xFF >> bitOffset;
			uint8_t mask2 = 0xFF << (6 - bitOffset);
			if(!skipFirstOctet) curDectet = ((uint16_t)(payload[curOffset++] & mask1)) << (2 + bitOffset);
			if (curOffset >= payloadLen) {
				skipFirstOctet = true;
				continue;
			}
			curDectet += ((uint16_t)(payload[curOffset] & mask2)) >> (6-bitOffset);
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
			//TODO: Sync to frame starts and edges
			if (curDectet < 4 || curDectet > 1019 || ancStart || syncStart) {

				if (isHD) {
					//For HD Data, read every other packet (EAVs and SAVs must happen at the same time anyway)
					hdSplitter = !hdSplitter;
					if (hdSplitter) continue;
				}

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

						if (curDectet & (1 << 8)) {	//F bit = Interleaving
							interleaved = 1;
						}
						else {
							interleaved = 0;
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

			if (horizontalBlanking || verticalBlanking) continue;

			//Reduce the range to 8 bits by removing the 2 LSB
			curDectet = curDectet >> 2;

			//Transfer the data into the pixel buffer in the correct order
			//Cb -> U; Y -> Y; Cr -> V
			int pixelIndex = curSDIDataCount;
			pixelIndex += (curSDIDataCount % 2) ? -1 : 1;

			if (isInterlaced) {
				int pixelRow = pixelIndex / (width * 2);
				int pixelOffset = pixelIndex - (pixelRow * (width * 2));
				//pixelRow *= 2;

				if (interleaved) {
					pixelRow -= height / 2;
					pixelRow *= 2;
					pixelRow += 1;
				}
				else {
					pixelRow *= 2;
				}

				pixelIndex = pixelRow * width * 2;
				pixelIndex += pixelOffset;
			}

			if (pixelIndex > pixelBufLen || pixelIndex < 0) {
				continue;
			}

			pixelBuf[pixelIndex] = curDectet;
			curSDIDataCount++;
		}
		mPacketGetter.releasePacket(pkt);
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
