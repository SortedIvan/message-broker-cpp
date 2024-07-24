#pragma once
#include "SFML/Network.hpp"

class Client {
	std::string clientId;
	std::shared_ptr<sf::TcpSocket> clientSocket;
};