#pragma once
#include <stdint.h>
#include <fstream>
#include <thread>

#define BUFSIZE 10
#define PACKETSIZE 1437

struct pkt_ll {
	uint8_t pkt[PACKETSIZE];
	pkt_ll * next;
};

class packetGetter
{
public:
	packetGetter();
	pkt_ll * getNextPacket();
	void releasePacket(pkt_ll * toRelease);
	~packetGetter();
private:
	void lockFirstFrame();
	static void sFillBuffer(packetGetter * const context);
	void fillBuffer();
	void putPkt(pkt_ll * packet);
	void freePkt(pkt_ll * packet);
	pkt_ll * popFreePkt();
	pkt_ll * popPkt();
	bool isLocked = false;
	int prevPacketSeq = 0;
	std::ifstream instr;
	std::thread iothread;
	int curPackNo = 0;
};

