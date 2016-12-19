#include "windowManager.h"
#include "debugUtil.h"
#include "smpteForwarder.h"
#include "pcapGenerator.h"
#include <iostream>
#include <string>

static void printHelp() {
	std::cout << "Help:" << std::endl;
	std::cout << "view <filepath> -- view a SMPTE2022 pcap file as a video" << std::endl;
	std::cout << "fwd <dst-ip> <dst-port> <filepath> -- forward a SMPTE2022 pcap file over a network" << std::endl;
	std::cout << "gen <mode> <lengthSeconds> <filepath> -- generate a SMPTE2022 pcap file" << std::endl;
	std::cout << "                                      -- mode 1 = 720x486i @ 30/1.001 fps" << std::endl;
	std::cout << "help -- print this message" << std::endl;
	std::cout << "exit -- exit application" << std::endl;
}

int main() {
	loginfo("Begin.");

	printHelp();

	bool running = true;
	while (running) {
		std::string input;
		std::cout << ">";

		std::cin >> input;

		if (input.compare("view") == 0) {
			std::cin >> input;
			windowManager gui(input);
			gui.start();
		}
		else if (input.compare("fwd") == 0) {
			std::string dstIp, dstPort, filepath;
			std::cin >> dstIp >> dstPort >> filepath;
			smpteForwarder fwdr(dstIp, dstPort, filepath);
			fwdr.start();
		}
		else if (input.compare("gen") == 0) {
			int mode, length;
			std::string filepath;
			std::cin >> mode >> length >> filepath;
			pcapGenerator gen(mode, length, filepath);
			gen.start();
		}
		else if (input.compare("help") == 0) {
			printHelp();
		}
		else if (input.compare("exit") == 0) {
			return 0;
		}
	}

	return 0;
}
