#include "smpteForwarder.h"
#include "debugUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <cstring>

#ifdef _WIN64
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define SMPTE_2022_PACKET_LENGTH 1438

smpteForwarder::smpteForwarder(std::string ipStr, std::string portStr, std::string filepath):
	mPacketGetter(filepath)
{
	this->portStr = portStr;
	port = std::stoi(portStr);
	ipAddr = ipStr;
}

void smpteForwarder::start()
{

#ifdef _WIN64
	SOCKET mSocket;
	WSADATA wsa;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);

	//Initialise winsock
	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//create socket
	if ((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	inet_pton(AF_INET , ipAddr.c_str(), &si_other.sin_addr.S_un.S_addr);

	//u_long nonzero = 1;
	//ioctlsocket(mSocket, FIONBIO, &nonzero); //Set socket to nonblocking

#else
	int sockfd;
	socklen_t salen;
	struct sockaddr mSockAddr;
	struct addrinfo hints, *res, *ressave;

	/*initilize addrinfo structure*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if ((getaddrinfo(ipAddr.c_str(), portStr.c_str(), &hints, &res)) != 0) {
		printf("Error filling address struct!\r\n");
	}
	ressave = res;

	do {/* each of the returned IP address is tried*/
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0)
			break; /*success*/
	} while ((res = res->ai_next) != NULL);

	memset(&mSockAddr, 0, sizeof(mSockAddr));
	memcpy(&mSockAddr, res->ai_addr, res->ai_addrlen);
	salen = res->ai_addrlen;

	freeaddrinfo(ressave);
#endif

	

	pkt_ll * curPkt;
	uint32_t originTime = 0;
	
	curPkt = mPacketGetter.getNextPacket();

	std::this_thread::sleep_for(std::chrono::seconds(1)); //Allow buffer to fill

	std::chrono::time_point<std::chrono::high_resolution_clock> begin = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> next;

	originTime += ((uint32_t) curPkt->pkt[46]) << 24;
	originTime += ((uint32_t) curPkt->pkt[47]) << 16;
	originTime += ((uint32_t) curPkt->pkt[48]) << 8;
	originTime += curPkt->pkt[49];

	while (1) {

		uint32_t curTime = 0;
		curTime += ((uint32_t)curPkt->pkt[46]) << 24;
		curTime += ((uint32_t)curPkt->pkt[47]) << 16;
		curTime += ((uint32_t)curPkt->pkt[48]) << 8;
		curTime += curPkt->pkt[49];

		//Convert the timestamp (in terms of a 27MHz clock) to nanoseconds since origin
		uint64_t nextTime = (uint64_t)1000 * (uint64_t)(curTime - originTime);
		nextTime /= (uint64_t) 27;

		next = begin + std::chrono::nanoseconds(nextTime);
		std::this_thread::sleep_until(next);

		//Forward packet
#ifdef _WIN64
		//TODO: 42 is magic number which cuts out all of the existing ethernet and UDP header
		bool retry = true;
		while (retry) {
			retry = false;
			if (sendto(mSocket, (char *)(curPkt->pkt + 42), SMPTE_2022_PACKET_LENGTH, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err = WSAEWOULDBLOCK) {
					retry = true;
					continue;
				}
				printf("sendto() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}
		}
#else
		//TODO: 42 is magic number which cuts out all of the existing ethernet and UDP header
		bool retry = true;
		while (retry) {
			retry = false;
			if (sendto(sockfd, (curPkt->pkt + 42), SMPTE_2022_PACKET_LENGTH, 0, &mSockAddr, salen) < 0) {
				/* buffers aren't available locally at the moment,
				* try again.
				*/
				if (errno == ENOBUFS) {
					retry = true;
					continue;
				}
				printf("sendto() failed with error code : %d", errno);
				exit(1);
			}
		}
#endif

		//Get next packet
		mPacketGetter.releasePacket(curPkt);
		curPkt = mPacketGetter.getNextPacket();

		if (curPkt == NULL) {
			next = std::chrono::high_resolution_clock::now();
			int millis = std::chrono::duration_cast<std::chrono::milliseconds>(next - begin).count();

			printf("Total millis to run: %d", millis);
			return;
		}

	}
}

smpteForwarder::~smpteForwarder()
{
}
