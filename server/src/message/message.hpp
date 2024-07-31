#pragma once
#include <string>
#include <vector>
#include <memory>

struct ConnectionMessage {
	bool isCorrect = false;
	std::string clientId;
	std::vector<std::string> publisherTo;     // In the packet, these two are seperated by a "|"
	std::vector<std::string> subscriberTo;
};

struct Header {
	std::string content; // template for now
};

struct Message {
	bool isCorrect = false;
	std::string content;
	std::vector<Header> headers;
};