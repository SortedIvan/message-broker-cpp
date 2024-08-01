#pragma once
#include "SFML/Network.hpp"
#include <queue>
#include "../src/message/message.hpp"

struct Topic {
	int maxAllowedConnections;
	std::string topicId;
	std::queue<Message> messages;
};