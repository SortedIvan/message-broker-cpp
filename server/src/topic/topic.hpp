#pragma once
#include "SFML/Network.hpp"
#include <queue>

struct Topic {
	int maxAllowedConnections;
	std::string topicId;
	std::queue<std::string> messages;
};