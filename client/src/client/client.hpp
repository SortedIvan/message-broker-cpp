#pragma once
#include "SFML/Network.hpp"
#include <string>
#include <vector>

struct Client {
	std::string clientId;
	std::shared_ptr<sf::TcpSocket> clientSocket;
	std::vector<std::string> messageSentHistory;
	std::vector<std::string> messageReceivedHistory;
	std::vector<std::string> subscriberTo;
	std::vector<std::string> publisherTo;
	bool clientInitialized = false;
};