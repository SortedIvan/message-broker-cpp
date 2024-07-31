#pragma once
#include "SFML/Network.hpp"
#include <string>
#include <vector>
#include <unordered_set>

struct Client {
	std::string clientId;
	std::shared_ptr<sf::TcpSocket> clientSocket;
	std::vector<std::string> messageSentHistory;
	std::vector<std::string> messageReceivedHistory;
	std::unordered_set<std::string> subscriberTo;
	std::unordered_set<std::string> publisherTo;
	bool clientInitialized = false;
};