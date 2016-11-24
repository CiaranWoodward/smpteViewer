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
