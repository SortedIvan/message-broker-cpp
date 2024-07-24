#pragma once
#include <string>
#include <vector>

struct ConnectionMessage {
	bool isCorrect = false;
	std::string clientId;
	std::vector<std::string> publisherTo;     // In the packet, these two are seperated by a "|"
	std::vector<std::string> subscriberTo;
};