#pragma once
#include <string>
#include <vector>
#include <memory>
#include "msgpack.hpp"
#include "../src/json.hpp"


// Well known pre-defined enumerator of message action type
// Has to be kept consistent between the server and the client
enum MessageActionType {
	None = 0,
	Connect = 1,
	Disconnect = 2, 
	SimpleMessage = 3,
	ServerHasShutdown = 4
};

struct Header {
	std::string content; // template for now
};

struct Message {
	std::string sender;
	int messageActionType;
	std::string actionData;
	std::vector<char> headers;
	MSGPACK_DEFINE(sender, messageActionType, actionData, headers);
};

struct ConnectionData {
	std::vector<std::string> publisherTo;
	std::vector<std::string> subscriberTo;
	MSGPACK_DEFINE(publisherTo, subscriberTo);
};