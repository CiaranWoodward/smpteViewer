#pragma once
#include <stdint.h>

class imagePacker
{
public:
	imagePacker();
	void init(int width, int height);
	uint8_t* getNextPixels();
	~imagePacker();
private:
	uint8_t * pixelBuf;
	int width, height, pixelBufLen;

};

