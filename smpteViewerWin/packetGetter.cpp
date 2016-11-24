#include "packetGetter.h"

//TODO: Include the ridiculous array of ethernet frames here
uint8_t ** pkts;

#define SEARCH_MARGIN 5

packetGetter::packetGetter()
{
}

uint8_t * packetGetter::getNextPacket()
{
	bool found = false;

	//Lock to stream if not already
	if (!isLocked) lockFirstFrame();

	//Then find the next packet needed
	if (pkts[curPackNo][45] == prevPacketSeq + 1) {
		prevPacketSeq++;
		found = true;
	}
	else {
		for (int i = 1; i < SEARCH_MARGIN; i++) {
			if (pkts[curPackNo + i][45] == prevPacketSeq + 1) {
				curPackNo = curPackNo + i;
				prevPacketSeq++;
				found = true;
				break;
			}
			else if (pkts[curPackNo - i][45] == prevPacketSeq + 1) {
				curPackNo = curPackNo - i;
				prevPacketSeq++;
				found = true;
				break;
			}
		}
	}

	//Return the next packet, or if all hope is lost, skip the rest of this frame and start the next one (yes, this is overly aggressive)
	if (found) {
		return pkts[curPackNo];
	}
	else {
		isLocked = false;
		return getNextPacket();
	}
}


packetGetter::~packetGetter()
{
}

void packetGetter::lockFirstFrame()
{
	while (!isLocked) {
		if (pkts[curPackNo][62] == 0xFF &&
			pkts[curPackNo][63] == 0xC0 &&
			pkts[curPackNo][64] == 0x00 &&
			pkts[curPackNo][65] == 0x02)
		{
			isLocked = true;
			prevPacketSeq = (pkts[curPackNo][45]) - 1;
			
			break;
		}
		curPackNo++;
	}
}
