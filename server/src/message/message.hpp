#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../src/json.hpp"


// Well known pre-defined enumerator of message action type
// Has to be kept consistent between the server and the client
enum MessageActionType {
	None = 0,
	Connect = 1,
	Disconnect = 2, 
	SimpleMessage = 3
};

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
	MessageActionType messageActionType;
	nlohmann::json content;
	std::vector<Header> headers;
};

