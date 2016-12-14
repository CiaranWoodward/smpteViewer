#include "packetGetter.h"

packetGetter::packetGetter(std::string & filepath) :
	iothread(&sFillBuffer, this, filepath)
{
	isLocked = false;
	prevPacketSeq = 0;
	curPackNo = 0;

	stash_pkt_head = NULL;
	stash_pkt_tail = NULL;

	reset = false;
	exitFlag = false;

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
			prevPacketSeq = (prevPacketSeq + 1) % 256;
			found = true;
		}
		else {
			putStash(curPkt);
		}

		if (curPkt->isEOF) {
			popStash();
			while (pkt_head != NULL) {
				curPkt = popPkt();
				freePkt(curPkt);
			}
			isLocked = false;
			prevPacketSeq = 0;
			reset = false;
			return NULL;
		}
	}

	popStash();

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

void packetGetter::returnPacket(pkt_ll * toReturn)
{
	if (toReturn == NULL) return;
	prevPacketSeq--;
	putPkt(toReturn);
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

void packetGetter::sFillBuffer(packetGetter * const context, std::string filepath)	//Copy filepath, not ref
{
	context->fillBuffer(filepath);
}

void packetGetter::fillBuffer(std::string & filepath)
{
	while (1) {
		std::ifstream instr = std::ifstream(filepath, std::ios::in | std::ios::binary);
		while (reset || exitFlag) {
			if (exitFlag) return;
		}

		uint8_t header[24];
		instr.seekg(0, std::ios::beg);
		instr.read((char *)header, 24);
		if (!(header[0] == 0xd4 &&
			header[1] == 0xc3 &&
			header[2] == 0xb2 &&
			header[3] == 0xa1))
		{
			return;	//Not a valid pcap file
		}
		while (1) {
			if (exitFlag) return;
			struct pkt_ll * tempPkt = popFreePkt();

			//Fill packet
			instr.read((char *)header, 16);
			uint32_t length = ((uint32_t)header[8]) | (((uint32_t)header[9]) << 8) | (((uint32_t)header[10]) << 16) | (((uint32_t)header[11]) << 24);
			if (length != 1438) {
				instr.ignore(length, EOF);
			}
			else {
				instr.read((char *)tempPkt->pkt, length);
			}

			if (instr.eof() || instr.fail()) {
				reset = true;
				tempPkt->next = NULL;
				tempPkt->isEOF = true;
				putPkt(tempPkt);
				break;
			}

			tempPkt->isEOF = false;
			tempPkt->next = NULL;

			putPkt(tempPkt);
		}
	}
}

void packetGetter::putStash(pkt_ll * packet) {
	if (stash_pkt_head == NULL) {
		stash_pkt_head = packet;
	}
	else {
		stash_pkt_tail->next = packet;
	}
	stash_pkt_tail = packet;
}

void packetGetter::popStash() {
	if (stash_pkt_head == NULL) return;

	pkt_head_mutex.lock();
	if (pkt_head == NULL) {
		pkt_head = stash_pkt_head;
		pkt_tail = stash_pkt_tail;
	}
	else {
		stash_pkt_tail->next = pkt_head;
		pkt_head = stash_pkt_head;
	}

	stash_pkt_head = NULL;
	stash_pkt_tail = NULL;
	pkt_head_mutex.unlock();
	pkt_head_cv.notify_one();
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
	exitFlag = true;

	popStash();
	if (free_pkt_head == NULL) {
		freePkt(popPkt());
	}

	iothread.join();
	
	while (pkt_head != NULL) {
		freePkt(popPkt());
	}

	while (free_pkt_head != NULL) {
		delete popFreePkt();
	}
}
