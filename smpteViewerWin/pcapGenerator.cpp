#include "pcapGenerator.h"
#include "debugUtil.h"
#include <time.h>
#include <assert.h>
#include "CRC.h"

#define BLANKLUMA 0x040
#define BLANKCHROMA 0x200

#define PUTLE32(x,y) do{(y)[0]=((x)&0xff);(y)[1]=(((x)>>8)&0xff);(y)[2]=(((x)>>16)&0xff);(y)[3]=(((x)>>24)&0xff);}while(0)
#define PUTLE16(x,y) do{(y)[0]=((x)&0xff);(y)[1]=(((x)>>8)&0xff);}while(0)

#define PUTBE32(x,y) do{(y)[3]=((x)&0xff);(y)[2]=(((x)>>8)&0xff);(y)[1]=(((x)>>16)&0xff);(y)[0]=(((x)>>24)&0xff);}while(0)
#define PUTBE16(x,y) do{(y)[1]=((x)&0xff);(y)[0]=(((x)>>8)&0xff);}while(0)

static const CRC::Parameters<uint32_t, 18> crcParameters = { 0x31, 0x0000, 0x0000, false, false };


pcapGenerator::pcapGenerator(int mode, int timeSec, std::string filepath) :
	bitOffset(0),
	curDectetCount(0),
	curFrameCount(0),
	sequenceNumber(0)
{
	this->mode = mode;
	this->filepath = filepath;
	if (mode == 1) {
		xDim = 720;
		yDim = 486;
		isHD = false;
		numPixels = xDim * yDim * 2;
		mImageGenerator = new imageGenerator(xDim, yDim);
		clockSpeed = 27000000;
		dectetsPerFrame = 900900; //(clockSpeed) / (30/1.001)
		dectetsPerLine = 1716;
		hBlankLen = 276 - (4 * 2); //subtract the length of the EAV and SAV markers 
		isInterlaced = true;
		maxFrames = timeSec * 30; //WARN: Doesn't take the 1.001 into account
	}
	if (mode == 2) {
		xDim = 1920;
		yDim = 1080;
		isHD = true;
		chromaBuf = (uint8_t *) malloc((xDim * 10) / 8);
		lumaBuf = (uint8_t *)malloc((xDim * 10) / 8);
		numPixels = xDim * yDim * 2;
		mImageGenerator = new imageGenerator(xDim, yDim);
		clockSpeed = 74250000; //For 30FPS - only clock rate changes for /1.001 version
		dectetsPerFrame = 2475000; //(clockSpeed) / (30)
		dectetsPerLine = 2200;
		hBlankLen = 280 - (4 * 2); //subtract the length of the EAV and SAV markers 
		isInterlaced = true;
		maxFrames = timeSec * 30;
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

	//open file and write pcap header
	outstr = std::ofstream(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

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

	startTime = time(0);

	uint8_t pktHeader[pktHeaderLength] = { 0x01, 0x00, 0x5e, 0x00, 0x27, 0x7a, 0x40, 0xa3, 0x6b, 0xa0, 0x01, 0xfe, 0x08, 0x00, 0x45, 0x02, 0x05, 0x90, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x36, 0xbe, 0xc0, 0xa8, 0x27, 0x7a, 0xef, 0x00, 0x27, 0x7a, 0x27, 0x10, 0x4e, 0x20, 0x05, 0x7c, 0x00, 0x00 };
	uint8_t rtpHeader[rtpHeaderLength] = { 0x80, 0x62, 0xf8, 0x66, 0xf2, 0x93, 0xc4, 0x05, 0x12, 0x34, 0x56, 0x00 };
	uint8_t hbrmHeader[hbrmHeaderLength] = { 0x08, 0x00, 0x00, 0x00, 0x01, 0x01, 0x71, 0x00 };

	if (mode == 2) {
		hbrmHeader[4] = (hbrmHeader[4] & 0xF0) + 0x02;	//1080i
		hbrmHeader[5] &= 0x0F;
		hbrmHeader[5] = (hbrmHeader[5] & 0xF0) + 0x06;	//30FPS
		hbrmHeader[6] = (hbrmHeader[6] & 0x0F) + 0x10;
	}

	memset(pkt, 0, PACKETSIZE);
	memcpy(pkt, pktHeader, pktHeaderLength);
	memcpy(pkt + pktHeaderLength, rtpHeader, rtpHeaderLength);
	memcpy(pkt + pktHeaderLength + rtpHeaderLength, hbrmHeader, hbrmHeaderLength);
	pktCursor = pktHeaderLength + rtpHeaderLength + hbrmHeaderLength;
	resetPacket();

	int numVblankLines = (dectetsPerFrame / dectetsPerLine) - yDim;
	resetCRCBufs();

	while (1) {
		uint32_t curPixel = 0;
		uint32_t curPacketByte = pktHeaderLength;
		pixels = mImageGenerator->getNextFrame();

		SDL_UpdateTexture(texture, NULL, pixels, xDim * 2);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		//process image into packets and write to file
		//TODO: modify to also do non-interlaced video

		int vb[4];
		vb[0] = (numVblankLines - 5) / 2;
		vb[1] = 2;
		vb[2] = ((numVblankLines - 5) / 2) + 1;
		vb[3] = numVblankLines - (vb[0] + vb[1] + vb[2]);
		assert(vb[3] >= 0);

		lineNo = 0;

		//VB
		for (int i = 0; i < vb[0]; i++) {
			pushEAV(false, true);
			pushHorizBlankData();
			pushSAV(false, true);
			pushVerticalBlankingLine();
		}

		//Video data evens
		for (int i = 0; i < (yDim-1); i += 2) {
			pushEAV(false, false);
			pushHorizBlankData();
			pushSAV(false, false);

			//Video line data
			unsigned int offset = i * xDim * 2;
			for (int x = 0; x < xDim; x++) {
				unsigned int index = offset + (x * 2);
				pushChromaDectet(((uint16_t)pixels[index + 1]) << 2);
				pushLumaDectet(((uint16_t)pixels[index]) << 2);
			}
		}

		//VB
		for (int i = 0; i < vb[1]; i++) {
			pushEAV(false, true);
			pushHorizBlankData();
			pushSAV(false, true);
			pushVerticalBlankingLine();
		}

		for (int i = 0; i < vb[2]; i++) {
			pushEAV(true, true);
			pushHorizBlankData();
			pushSAV(true, true);
			pushVerticalBlankingLine();
		}

		// Video data odds
		for (int i = 1; i < (yDim - 1); i += 2) {
			pushEAV(true, false);
			pushHorizBlankData();
			pushSAV(true, false);

			//Video line data
			unsigned int offset = i * xDim * 2;
			for (int x = 0; x < xDim; x++) {
				unsigned int index = offset + (x * 2);
				pushChromaDectet(((uint16_t)pixels[index + 1]) << 2);
				pushLumaDectet(((uint16_t)pixels[index]) << 2);
			}
		}

		//VB
		for (int i = 0; i < vb[3]; i++) {
			pushEAV(true, true);
			pushHorizBlankData();
			pushSAV(true, true);
			pushVerticalBlankingLine();
		}

		pkt[43] |= 0x80; //Set marker to signal final packet for this frame
		pushPacket();
		bitOffset = 0;
		pkt[43] &= (~0x80);
		curFrameCount++;
		
		if (curFrameCount > maxFrames) {
			SDL_DestroyTexture(texture);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(screen);
			SDL_Quit();
			return;
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

void pcapGenerator::resetPacket()
{
	//reset cursor zero the data section of packet ready for next filling
	pktCursor = pktHeaderLength + rtpHeaderLength + hbrmHeaderLength;
	memset(pkt + pktCursor, 0, PACKETSIZE - pktCursor);

	//Initialise the rtp header with correct sync data
	PUTBE16(sequenceNumber, (pkt + 44));
	uint64_t timestamp = curDectetCount & 0xFFFFFFFF;
	if (clockSpeed == 74250000) {
		//Make RTP timestamp relative to 27MHz clock (divide by 2.75)
		timestamp = timestamp * 4;
		timestamp = timestamp / 11; 
	}
	if (isHD) timestamp /= 2;
	PUTBE32(timestamp, (pkt + 46));

	//Frame count into the hbrm header
	pkt[55] = curFrameCount & 0xFF;
}

void pcapGenerator::pushPacket()
{
	//Form Pcap packet header
	uint8_t header[16] = {
		0x00, 0x00, 0x00, 0x00, //Timestamp seconds
		0x00, 0x00, 0x00, 0x00, //Timestamp microSeconds
		0x00, 0x00, 0x00, 0x00, //Included packet length
		0x00, 0x00, 0x00, 0x00 //Original Packet Length
	};

	unsigned long long int nanos = (unsigned long long int)((curDectetCount * 1000) / (clockSpeed/1000000));
	if (isHD) nanos /= 2;
	const unsigned long long int bil = 1000000000;
	const unsigned long long int mil = 1000000;

	unsigned long long int secs = (nanos / bil);
	unsigned long long int uSecs = ((nanos/1000) - (secs * mil));
	//secs += startTime;

	PUTLE32(secs, (header));
	PUTLE32(uSecs, (header + 4));
	PUTLE32(PACKETSIZE, (header + 8));
	PUTLE32(PACKETSIZE, (header + 12));

	//Write Pcap packet header
	outstr.write((char *)header, 16);

	//Write packet data
	outstr.write((char *)pkt, PACKETSIZE);

	sequenceNumber++;
	resetPacket();
}

void pcapGenerator::resetCRCBufs() {
	if (isHD) {
		memset(chromaBuf, 0, sizeof(chromaBuf));
		memset(lumaBuf, 0, sizeof(lumaBuf));
		chromaCount = 0;
		lumaCount = 0;
	}
}

void pcapGenerator::pushChromaDectet(uint16_t dectet)
{
	if (isHD) {
		uint8_t toInsert[2];
		uint8_t mask[2];

		uint8_t bitOffset = (chromaCount % 4) * 2;

		uint32_t index = (chromaCount * 10) / 8;

		mask[0] = 0xFF >> bitOffset;
		mask[1] = 0xFF << (6 - bitOffset);

		toInsert[0] = (dectet >> (2 + bitOffset)) & mask[0];
		toInsert[1] = (dectet << (6 - bitOffset)) & mask[1];

		for (int i = 0; i < 2; i++) {
			//Put toInsert[i] in the buffer (using bitwise or so as to not disrupt existing data)
			chromaBuf[index] |= toInsert[i];
			if (mask[i] & 0x01) index++;
		}

		chromaCount++;
	}
	pushDectet(dectet);
}

void pcapGenerator::pushLumaDectet(uint16_t dectet)
{
	if (isHD) {
		uint8_t toInsert[2];
		uint8_t mask[2];

		uint8_t bitOffset = (lumaCount % 4) * 2;

		uint32_t index = (lumaCount * 10) / 8;

		mask[0] = 0xFF >> bitOffset;
		mask[1] = 0xFF << (6 - bitOffset);

		toInsert[0] = (dectet >> (2 + bitOffset)) & mask[0];
		toInsert[1] = (dectet << (6 - bitOffset)) & mask[1];

		for (int i = 0; i < 2; i++) {
			//Put toInsert[i] in the buffer (using bitwise or so as to not disrupt existing data)
			lumaBuf[index] |= toInsert[i];
			if (mask[i] & 0x01) index++;
		}

		lumaCount++;
	}
	pushDectet(dectet);
}

void pcapGenerator::pushDectet(uint16_t dectet)
{
	uint8_t toInsert[2]; 
	uint8_t mask[2]; 

	mask[0] = 0xFF >> bitOffset;
	mask[1] = 0xFF << (6 - bitOffset);

	toInsert[0] = (dectet >> (2 + bitOffset)) & mask[0];
	toInsert[1] = (dectet << (6 - bitOffset)) & mask[1];

	for (int i = 0; i < 2; i++) {
		if (pktCursor < PACKETSIZE) {
			//Put toInsert[i] in this packet (using bitwise or so as to not disrupt existing data)
			pkt[pktCursor] |= toInsert[i];
			if (mask[i] & 0x01) pktCursor++;
		}
		else {
			pushPacket();
			i--;
		}
	}

	bitOffset += 2;
	if (bitOffset == 8) {
		bitOffset = 0;
	}

	curDectetCount++;
}

void pcapGenerator::pushVerticalBlankingLine()
{
	bool blankLumaToggle = false;
	for (int i = 0; i < (xDim*2); i++) {
		blankLumaToggle ? pushLumaDectet(BLANKLUMA) : pushChromaDectet(BLANKCHROMA);
		blankLumaToggle = !blankLumaToggle;
	}
}

void pcapGenerator::pushHorizBlankData()
{
	int filler = hBlankLen;

	if (isHD) {
		uint16_t ln0, ln1, ycr0, ycr1, ccr0, ccr1;

		ln0 = (lineNo & 0x07FF) << 2;
		ln1 = (lineNo & 0x7800) >> 9;
		
		ln0 |= (ln0 & 0x0100) ? 0x00 : 0x0200;
		ln1 |= (ln1 & 0x0100) ? 0x00 : 0x0200;

		pushDectet(ln0); pushDectet(ln0); filler--;
		pushDectet(ln1); pushDectet(ln1); filler--;

		//TODO: Fill in CRC dectets
		uint32_t lumaCRC = CRC::Calculate(lumaBuf, sizeof(lumaBuf), crcParameters);
		uint32_t chromaCRC = CRC::Calculate(chromaBuf, sizeof(chromaBuf), crcParameters);

		ycr0 = (lumaCRC & 0x1FF);
		ycr1 = ((uint32_t)(lumaCRC >> 9) & 0x1FF);

		ccr0 = (chromaCRC & 0x1FF);
		ccr1 = ((uint32_t)(chromaCRC >> 9) & 0x1FF);

		ycr0 |= (ycr0 & 0x0100) ? 0x00 : 0x0200;
		ycr1 |= (ycr1 & 0x0100) ? 0x00 : 0x0200;

		ccr0 |= (ccr0 & 0x0100) ? 0x00 : 0x0200;
		ccr1 |= (ccr1 & 0x0100) ? 0x00 : 0x0200;

		pushDectet(ccr0);
		pushDectet(ycr0); filler--;
		pushDectet(ccr1);
		pushDectet(ycr1); filler--;

		resetCRCBufs();
	}

	bool blankLumaToggle = false;
	if (isHD) filler *= 2;
	for (int i = 0; i < filler; i++) {
		blankLumaToggle ? pushDectet(BLANKLUMA) : pushDectet(BLANKCHROMA);
		blankLumaToggle = !blankLumaToggle;
	}
}

void pcapGenerator::pushSAV(bool f, bool v)
{
	pushDectet(0x3FF);
	if(isHD) pushDectet(0x3FF);
	pushDectet(0);
	if (isHD) pushDectet(0);
	pushDectet(0);
	if (isHD) pushDectet(0);
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
	if (isHD) pushDectet(xyz);
}

void pcapGenerator::pushEAV(bool f, bool v)
{
	lineNo++;
	pushDectet(0x3FF);
	if (isHD) pushDectet(0x3FF);
	pushDectet(0);
	if (isHD) pushDectet(0);
	pushDectet(0);
	if (isHD) pushDectet(0);
	uint16_t xyz = 0;
	if (f) {
		if (v) {
			      //1fvhpppp00
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
	if (isHD) pushDectet(xyz);
}

pcapGenerator::~pcapGenerator()
{
	if (chromaBuf) free(chromaBuf);
	if (lumaBuf) free(lumaBuf);
	delete mImageGenerator;
}
