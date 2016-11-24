#pragma once
#include <stdint.h>

class packetGetter
{
public:
	packetGetter();
	uint8_t * getNextPacket();
	~packetGetter();
private:
	void lockFirstFrame();
	bool isLocked = false;
	int prevPacketSeq = 0;
	int curPackNo = 0;
};

