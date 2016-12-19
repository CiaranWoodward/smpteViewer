#include "imageGenerator.h"
#include <stdlib.h>



imageGenerator::imageGenerator(int xDim, int yDim)
{
	this->xDim = xDim;
	this->yDim = yDim;
	numPixels = xDim * yDim * 2;
	pixels =(uint8_t *) malloc(numPixels);
}

uint8_t * imageGenerator::getNextFrame()
{
	for (int y = 0; y < yDim; y++) {
		for (int x = 0; x < (xDim * 2); x += 2) {

			if (y < (yDim / 2)) {	//Top half of image
				if (x < xDim) {
					pixels[y*xDim*2 + x] = 20;	//luma
					pixels[y*xDim*2 + x + 1] = x % 4 ? 200 : 30; //Chroma
				}
				else {
					pixels[y*xDim*2 + x] = 150;	//luma
					pixels[y*xDim*2 + x + 1] = x % 4 ? 30 : 200; //Chroma
				}
			}
			else {	//Bottom half of image
				if ((x/2) < (counter % xDim)) {
					pixels[y*xDim*2 + x] = 100;	//luma
					pixels[y*xDim*2 + x + 1] = 200; //Chroma
				}
				else {
					pixels[y*xDim*2 + x] = 200;	//luma
					pixels[y*xDim*2 + x + 1] = 100; //Chroma
				}
			}
		}
	}

	counter++;

	return pixels;
}


imageGenerator::~imageGenerator()
{
	free(pixels);
}
