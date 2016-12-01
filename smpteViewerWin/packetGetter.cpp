#include <thread>
#include <condition_variable>
#include <mutex>

#include "packetGetter.h"

//TODO: Include the ridiculous array of ethernet frames here
//uint8_t ** pkts;

struct pkt_ll * pkt_head;
struct pkt_ll * pkt_tail;
std::condition_variable pkt_head_cv;
std::mutex pkt_head_mutex;

struct pkt_ll * free_pkt_head;
std::condition_variable free_pkt_head_cv;
std::mutex free_pkt_head_mutex;


packetGetter::packetGetter() :
	instr("C:\\Users\\cw15g13\\pcap_in.pcap", std::ios::in | std::ios::binary),
	iothread(&sFillBuffer, this)
{
	free_pkt_head = NULL;
	pkt_head = NULL;

	for (int i = 0; i < BUFSIZE; i++) {
		freePkt(new pkt_ll());
	}
}

pkt_ll * packetGetter::getNextPacket()
{
	bool found = false;

	//Lock to stream if not already
	if (!isLocked) lockFirstFrame();

	//Then find the next packet needed
	struct pkt_ll * curPkt = NULL;

	while (!found) {
		//Get packet
		curPkt = popPkt();

		if (curPkt->pkt[45] == (prevPacketSeq + 1) % 256) {
			prevPacketSeq++;
			found = true;
		}
		else {
			//TODO: Don't add it right to the back of the queue
			putPkt(curPkt);
		}
	}

	//Return the next packet, or if all hope is lost, skip the rest of this frame and start the next one (yes, this is overly aggressive)
	if (found) {
		return curPkt;
	}
	else {
		isLocked = false;
		return getNextPacket();
	}
}

void packetGetter::releasePacket(pkt_ll * toRelease)
{
	freePkt(toRelease);
}


//TODO: Current limitation -For the first frame, if any frame packets are received before the TRS preamble, they will be lost
void packetGetter::lockFirstFrame()
{
	while (!isLocked) {
		pkt_ll * cur = popPkt();
		if (cur->pkt[62] == 0xFF &&	//Look for TRS Preamble which is 0x3FF 0x000 0x000 (ten bit hex)
			cur->pkt[63] == 0xC0 &&
			cur->pkt[64] == 0x00 &&
			(cur->pkt[65] & 0xFC) == 0x00)
		{
			isLocked = true;
			prevPacketSeq = (cur->pkt[45]) - 1;
			putPkt(cur);
			
			break;
		}
		freePkt(cur);
	}
}

void packetGetter::sFillBuffer(packetGetter * const context)
{
	context->fillBuffer();
}

void packetGetter::fillBuffer()
{
	while (!instr) std::this_thread::sleep_for(std::chrono::milliseconds(100));

	uint8_t header[24];
	instr.seekg(0, std::ios::beg);
	instr.read((char *) header, 24);
	if(!(header[0] == 0xd4 &&
		header[1] == 0xc3 &&
		header[2] == 0xb2 &&
		header[3] == 0xa1))
	{
		return;	//Not a valid pcap file
	}
	while (1) {
		struct pkt_ll * tempPkt = popFreePkt();

		//Fill packet
		instr.read((char *)header, 16);
		uint32_t length = ((uint32_t)header[8]) | (((uint32_t)header[9]) << 8) | (((uint32_t)header[10]) << 16 )| (((uint32_t)header[11]) << 24);
		if (length != 1438) {
			instr.ignore(length, EOF);
		}
		else {
			instr.read((char *)tempPkt->pkt, length);
		}

		tempPkt->next = NULL;

		putPkt(tempPkt);
	}
}

void packetGetter::putPkt(pkt_ll * packet)
{
	pkt_head_mutex.lock();
	if (pkt_head == NULL) {
		pkt_head = packet;
	}
	else {
		pkt_tail->next = packet;
	}
	pkt_tail = packet;
	pkt_head_mutex.unlock();
	pkt_head_cv.notify_one();
}

void packetGetter::freePkt(pkt_ll * packet)
{
	free_pkt_head_mutex.lock();
	packet->next = free_pkt_head;
	free_pkt_head = packet;
	free_pkt_head_mutex.unlock();
	free_pkt_head_cv.notify_one();
}

pkt_ll * packetGetter::popFreePkt()
{
	std::unique_lock<std::mutex> lock(free_pkt_head_mutex);
	while (free_pkt_head == NULL) free_pkt_head_cv.wait(lock);
	struct pkt_ll * tempPkt = free_pkt_head;
	free_pkt_head = free_pkt_head->next;
	lock.unlock();

	tempPkt->next = NULL;
	return tempPkt;
}

pkt_ll * packetGetter::popPkt()
{
	std::unique_lock<std::mutex> lock(pkt_head_mutex);
	while (pkt_head == NULL) pkt_head_cv.wait(lock);
	struct pkt_ll * tempPkt = pkt_head;
	pkt_head = tempPkt->next;
	if (pkt_head == NULL) pkt_tail = NULL;
	lock.unlock();

	tempPkt->next = NULL;
	return tempPkt;
}

packetGetter::~packetGetter()
{
	instr.close();
}
