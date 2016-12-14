#pragma once
#include <stdint.h>
#include <fstream>
#include <thread>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

#define BUFSIZE 100
#define PACKETSIZE 1437

struct pkt_ll {
	uint8_t pkt[PACKETSIZE];
	pkt_ll * next;
	bool isEOF;
};

class packetGetter
{
public:
	packetGetter(std::string & filepath);
	pkt_ll * getNextPacket();
	void releasePacket(pkt_ll * toRelease);
	void returnPacket(pkt_ll * toReturn);
	~packetGetter();
private:
	void lockFirstFrame();
	static void sFillBuffer(packetGetter * const context, std::string filepath);
	void fillBuffer(std::string & filepath);
	void putStash(pkt_ll * packet);
	void popStash();
	void putPkt(pkt_ll * packet);
	void freePkt(pkt_ll * packet);
	pkt_ll * popFreePkt();
	pkt_ll * popPkt();
	bool isLocked;
	int prevPacketSeq;
	std::thread iothread;
	int curPackNo;

	struct pkt_ll * pkt_head;
	struct pkt_ll * pkt_tail;
	std::condition_variable pkt_head_cv;
	std::mutex pkt_head_mutex;

	//Stash is for single threaded use only
	struct pkt_ll * stash_pkt_head;
	struct pkt_ll * stash_pkt_tail;

	struct pkt_ll * free_pkt_head;
	std::condition_variable free_pkt_head_cv;
	std::mutex free_pkt_head_mutex;

	std::atomic_bool reset;
	std::atomic_bool exitFlag;
};

