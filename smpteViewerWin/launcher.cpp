#include "windowManager.h"
#include "debugUtil.h"
#include <iostream>
#include <string>

int main() {
	loginfo("Begin.");

	std::cout << "Help:" << std::endl; 
	std::cout << "view <filepath> -- view a SMPTE2022 pcap file as a video" << std::endl;
	std::cout << "fwd <dst-ip> <dst-port> <filepath> -- forward a SMPTE2022 pcap file over a network" << std::endl;
	std::cout << "exit -- exit application" << std::endl;

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
		else if (input.compare("exit") == 0) {
			return 0;
		}
	}

	return 0;
}
