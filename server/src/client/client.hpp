#pragma once
#include "SFML/Network.hpp"
#include <string>
#include <unordered_set>

struct ConnectedClient {
	std::shared_ptr<sf::TcpSocket> clientSocket;
	std::unordered_set<std::string> publisherTo;
	std::unordered_set<std::string> subscriberTo;
};