#pragma once
#include <stdint.h>

class imageGenerator
{
public:
	imageGenerator(int xDim, int yDim);
	uint8_t * getNextFrame();
	~imageGenerator();
private:
	int xDim, yDim, numPixels, counter;
	uint8_t * pixels;
};

