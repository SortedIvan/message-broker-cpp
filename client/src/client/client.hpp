#pragma once
#include "SFML/Network.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include "../src/message/message.hpp"

#define CLIENT_ID_SIZE 4

struct Client {
	unsigned short port;
	sf::IpAddress serverIp;
	std::string clientId;
	std::vector<Message> messageSentHistory;
	std::vector<Message> messageReceivedHistory;
	std::unordered_set<std::string> subscriberTo;
	std::unordered_set<std::string> publisherTo;
	bool clientInitialized = false;
	bool clientIsConnected = false;
	std::shared_ptr<sf::TcpSocket> clientSocket;

	Client(std::string newClientId, std::string serverIp, unsigned short port);
	bool tryConnect();
	bool tryEstablishClientIdentity();
	bool signalDisconnect();
	void showAllReceived();
	void showAllSent();
	void printMessage(const Message& message);
	void sendMessage();
	bool processMessage(Message& message);
};