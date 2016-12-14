#pragma once

#include <string>
#include "packetGetter.h"


class smpteForwarder
{
public:
	smpteForwarder(std::string ipStr, std::string portStr, std::string filepath);
	void start();
	~smpteForwarder();
private:
	packetGetter mPacketGetter;
	uint32_t port;
	std::string ipAddr, portStr;
};

