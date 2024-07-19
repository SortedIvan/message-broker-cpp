#include "server.hpp"

Server::Server(std::string _ip, unsigned int _port) {
	ip = _ip;
	port = _port;
	serverIsRunning = false;
}

void Server::serverLoop() {
	serverIsRunning = true;
	
	if (listener.listen(port, ip) != sf::Socket::Done) {
		std::cerr << "Listener was not configured properly" << std::endl;
		return;
	}

	while (serverIsRunning) {
		sf::TcpSocket client;
		
		if (listener.accept(client) != sf::Socket::Done) {
		    std::cerr << "Error: Could not accept new connection" << std::endl;
			continue;
		}

		sf::Packet connectionPacket; // has a specific structure, should not connect player if it doesn't match

		// check the connection message from the client
		if (client.receive(connectionPacket) != sf::Socket::Done)
		{
			std::cerr << "Error: Could not processes connection packet from client" << std::endl;
			continue;
		}

		ConnectionMessage connectionMessage = parseConnectionPacket(connectionPacket);

		if (!connectionMessage.isCorrect) {
			continue;
		}
	}
}

ConnectionMessage Server::parseConnectionPacket(sf::Packet& connectionPacket) {
	ConnectionMessage connectionMessage;
	std::string data;

	if (!(connectionPacket >> data))
	{
		connectionMessage.isCorrect = false;
		return connectionMessage;
	}

	std::string clientId = "";
	std::vector<std::string> publisherTo;
	std::vector<std::string> subscriberTo;
	
	int dataPointer = 0;

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			break;
		}
		else {
			clientId.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	if (dataPointer != 4) {    // client id is exactly 4 characters
		connectionMessage.isCorrect = false;
		return connectionMessage;
	}
	
	std::string publishTo = "";

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|'){
			break;
		}
		else if (data[dataPointer] == ':') {

			if (publishTo.size() != 4) {
				connectionMessage.isCorrect = false;
				return connectionMessage;
			}

			publisherTo.push_back(publishTo);
			publishTo = "";
			dataPointer++;
		}
		else {
			publishTo.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	std::string subscribeTo = "";

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			break;
		}
		else if (data[dataPointer] == ':') {

			if (subscribeTo.size() != 4) {
				connectionMessage.isCorrect = false;
				return connectionMessage;
			}

			subscriberTo.push_back(subscribeTo);
			subscribeTo = "";
			dataPointer++;
		}
		else {
			subscribeTo.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	connectionMessage.isCorrect = true;
	connectionMessage.clientId = clientId;
	connectionMessage.publisherTo = publisherTo;
	connectionMessage.subscriberTo = subscriberTo;
	return connectionMessage;
}