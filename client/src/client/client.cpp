#include "client.hpp"
#include "../src/message/message.hpp"
#include "../src/message/message_serializer.hpp"
#include <iostream>
#include <thread>

Client::Client(std::string clientId, std::string serverIp, unsigned short port) {
	this->clientId = clientId;
	this->port = port;
	this->serverIp = sf::IpAddress(serverIp);
}

bool Client::tryConnect() {
	auto socket = std::make_shared<sf::TcpSocket>();

	sf::Socket::Status status = socket->connect(serverIp, port);
	if (status != sf::Socket::Done)
	{
		std::cerr << "Error connecting client" << std::endl;
		return false;
	}	
	socket->setBlocking(false);
	clientSocket = std::move(socket);
	return true;
}

bool Client::tryEstablishClientIdentity() {
	ConnectionData connectionData;
	connectionData.publisherTo = std::vector<std::string>(publisherTo.begin(), publisherTo.end());
	connectionData.subscriberTo = std::vector<std::string>(subscriberTo.begin(), subscriberTo.end());
	std::string serializedData = serialize<ConnectionData>(connectionData);

	if (serializedData.empty()) {
		return false;
	}

	Message connectionMessage;
	connectionMessage.sender = this->clientId;
	connectionMessage.messageActionType = (int)Connect;
	connectionMessage.actionData = serializedData;

	std::string serializedMessage = serialize<Message>(connectionMessage);

	sf::Packet connectionPacket;
	connectionPacket << serializedMessage;

	if (clientSocket->send(connectionPacket) != sf::Socket::Done) {
		std::cerr << "There was an error connecting the client. Please try again" << std::endl;
		return false;
	}

	std::cout << serializedMessage << std::endl;
	clientIsConnected = true;
	return true;
}

bool Client::signalDisconnect() {
	sf::Packet messagePacket;
	Message disconnectMessage;
	disconnectMessage.messageActionType = Disconnect;
	disconnectMessage.sender = clientId;
	std::string serializedMessage;

	try {
		serializedMessage = serialize<Message>(disconnectMessage);
		messagePacket << serializedMessage;

		if (clientSocket->send(messagePacket) != sf::Socket::Done) {
			std::cerr << "Something went wrong with disconnecting, please try again" << std::endl;
			return false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // sleep for 500 ms such that any potential unprocessed messages are handled

		clientIsConnected = false;
		return true;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		return false;
	}
}

void Client::showAllReceived() {
	for (int i = 0; i < messageReceivedHistory.size(); ++i) {
		printMessage(messageReceivedHistory[i]);
	}
}

void Client::printMessage(const Message& message) {
	std::cout << "---------- Message ------------" << std::endl;
	std::cout << "Sender: " << message.sender << std::endl;
	std::cout << "Message data" << message.actionData << std::endl;
}

void Client::showAllSent() {
	for (int i = 0; i < messageSentHistory.size(); ++i) {
		printMessage(messageReceivedHistory[i]);
	}
}

void Client::sendMessage() {
	sf::Packet messagePacket;
	std::string data = "";
	std::cout << "Input message data: ";
	std::getline(std::cin, data); // Read full line including spaces
	std::cout << std::endl;

	if (data.empty()) {
		std::cout << "No message entered." << std::endl;
		return;
	}

	Message message;
	message.sender = clientId;
	message.messageActionType = SimpleMessage;
	message.actionData = data;

	std::string serializedMessage = serialize<Message>(message);

	messagePacket << serializedMessage;

	if (clientSocket->send(messagePacket) != sf::Socket::Done) {
		std::cerr << "Problem with appending message to queue, breaking connection" << std::endl;
		clientIsConnected = false;
		return;
	}

	std::cout << "Message appended to queue successfully" << std::endl;
	messageSentHistory.push_back(message);
}

bool Client::processMessage(Message& message) {
	switch (message.messageActionType) {
		case None:
			return false;
		case SimpleMessage:
			std::cout << "Client: " << message.sender << " has sent {" << message.actionData << "}" << std::endl;
			return true;
		case Connect:
			return true;
		case Disconnect:
			std::cout << message.actionData << std::endl;
			return true;
		case ServerHasShutdown:
			clientIsConnected = false;
			std::cout << "Server has shutdown, terminating client" << std::endl;
	}
}
